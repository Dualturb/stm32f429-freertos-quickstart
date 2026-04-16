#include "flash.h"
#include "atk_flasher.h"
#include "main.h"
#include "cmsis_os.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_sdram.h"
#include <string.h>
#include <stdbool.h>

/* --- 1. Ring Buffer Definitions --- */
#define RX_BUF_SIZE 1024
static volatile uint8_t rx_ring_buffer[RX_BUF_SIZE];
volatile uint16_t head = 0;
volatile uint16_t tail = 0;
uint8_t uart_rx_byte; // This is the actual memory allocation

/* --- Hardware Handles & Globals --- */
extern UART_HandleTypeDef huart5;
osThreadId flashTaskHandle;

/* --- 2. Internal Function Prototypes --- */
static bool internal_flash_init(const Atk_FlasherInterface_t* interface);
static bool internal_flash_write(const Atk_FlasherInterface_t* interface, uint32_t address, const uint8_t* data, uint32_t size);
static bool internal_flash_read(const Atk_FlasherInterface_t* interface, uint32_t addr, uint8_t* buf, uint32_t size);
static bool internal_flash_erase_chip(const Atk_FlasherInterface_t* interface);
static uint32_t internal_get_size(const Atk_FlasherInterface_t* interface);
static void FLASH_Send_Response(uint8_t byte);
static void FLASH_Delay_Wrapper(uint32_t ms);

/* --- 3. Fully Defined Dummy Stubs (Defined BEFORE use in Struct) --- */
static void dummy_void(void) {}
static void dummy_select(void) {}
static void dummy_deselect(void) {}
static void dummy_transmit(const uint8_t* data, uint16_t size) { (void)data; (void)size; }
static void dummy_receive(uint8_t* data, uint16_t size) { (void)data; (void)size; }
static void dummy_transceive(const uint8_t* tx, uint8_t* rx, uint16_t size) { (void)tx; (void)rx; (void)size; }

static bool dummy_erase_sector(const Atk_FlasherInterface_t* interface, uint32_t address) {
    (void)interface; (void)address;
    return true;
}

/* --- 4. Interface & Driver Mapping (Compiler now sees all addresses) --- */
static const Atk_FlasherDriver_t internal_driver = {
    .init = internal_flash_init,
    .read = internal_flash_read,
    .write = internal_flash_write,
    .erase_sector = dummy_erase_sector,    // Resolved!
    .erase_chip = internal_flash_erase_chip,
    .get_size = internal_get_size
};

static const Atk_FlasherInterface_t internal_hw = {
    .preinit = dummy_void,
    .send_response_byte = FLASH_Send_Response,
    .device_select = dummy_select,
    .device_deselect = dummy_deselect,
    .transmit_data = dummy_transmit,
    .receive_data = dummy_receive,
    .transmit_receive = dummy_transceive,
    .delay_ms = FLASH_Delay_Wrapper
};

/* --- 5. Public API --- */

void FLASH_Init(void) {
    osThreadDef(flashTask, StartFlashTask, osPriorityAboveNormal, 0, 1024);
    flashTaskHandle = osThreadCreate(osThread(flashTask), NULL);

    if (flashTaskHandle == NULL) {
        HAL_GPIO_WritePin(GPIOG, LD4_Pin, GPIO_PIN_SET); // Solid Red on failure
    }
}

void FLASH_Feed_Byte(uint8_t byte) {
	HAL_GPIO_TogglePin(GPIOG, LD3_Pin);

	    uint16_t next = (head + 1) % RX_BUF_SIZE;
	    if (next != tail) {
	        rx_ring_buffer[head] = byte;
	        head = next;
	    }
}

void StartFlashTask(void const * argument) {
    char msg[32];
    uint32_t last_lcd_update = 0;

    __HAL_RCC_CRC_CLK_ENABLE();
    flasher_init(&internal_hw, &internal_driver);

    for(;;) {
        // 1. Process all bytes currently in the ring buffer
        while (tail != head) {
            uint8_t byte = rx_ring_buffer[tail];
            tail = (tail + 1) % RX_BUF_SIZE;
            flasher_process_byte(byte);
        }

        // 2. Run the internal state machine (handles timeouts/writes)
        flasher_process();

//        // 3. LCD Debug Update (Every 100ms)
//        if (osKernelSysTick() - last_lcd_update > 100) {
//            last_lcd_update = osKernelSysTick();
//
//            // Display buffer pointers and the last raw byte received
//            sprintf(msg, "H:%u T:%u B:0x%02X", head, tail, uart_rx_byte);
//            BSP_LCD_DisplayStringAt(0, 180, (uint8_t*)msg, CENTER_MODE);
//
//            // Use the getter function (defined in atk_flasher.c)
//            sprintf(msg, "State: %u", flasher_get_state());
//            BSP_LCD_DisplayStringAt(0, 210, (uint8_t*)msg, CENTER_MODE);
//        }

        osDelay(1); // Yield to other tasks
    }
}

/* --- 6. Driver Logic --- */

static bool internal_flash_init(const Atk_FlasherInterface_t* interface) {
    return true;
}

static bool internal_flash_erase_chip(const Atk_FlasherInterface_t* interface) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector       = FLASH_SECTOR_12;
    EraseInitStruct.NbSectors    = 1;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    HAL_FLASH_Lock();
    return (status == HAL_OK);
}

static bool internal_flash_write(const Atk_FlasherInterface_t* interface, uint32_t address, const uint8_t* data, uint32_t size) {
    HAL_FLASH_Unlock();
    // It is a best practice to clear flags before every write session
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    // STM32F4 writes in 32-bit words.
    // Ensure UI_FLASH_START_ADDR + address is word-aligned!
    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word = 0xFFFFFFFF;
        memcpy(&word, data + i, (size - i >= 4) ? 4 : (size - i));

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, UI_FLASH_START_ADDR + address + i, word) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }
    HAL_FLASH_Lock();
    return true;
}

static uint32_t internal_get_size(const Atk_FlasherInterface_t* interface) {
    return UI_FLASH_MAX_SIZE;
}

static bool internal_flash_read(const Atk_FlasherInterface_t* interface, uint32_t addr, uint8_t* buf, uint32_t size) {
    memcpy(buf, (void*)(UI_FLASH_START_ADDR + addr), size);
    return true;
}

/* --- 7. HW Wrappers --- */

static void FLASH_Send_Response(uint8_t byte) {
	HAL_UART_Transmit(&huart5, &byte, 1, 100);
}

static void FLASH_Delay_Wrapper(uint32_t ms) {
    osDelay(ms);
}
