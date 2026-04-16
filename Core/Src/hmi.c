/**
 * @file hmi.c
 * @brief Implementation of the HMI task and hardware abstraction for Artok.
 */

#include "hmi.h"
#include "atk_runtime.h"
#include "atk_api.h"
#include "flash.h"
#include "main.h"
#include <sys/time.h>

/* BSP Includes for STM32F429I-Discovery */
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

#include <string.h>
#include <stdio.h>

/* --- Global Handles --- */
osThreadId hmiTaskHandle;
osSemaphoreId hmiDma2dSemHandle; // Semaphore for DMA2D Sync
osSemaphoreDef(hmiDma2dSem);

extern DMA2D_HandleTypeDef hdma2d;
extern LTDC_HandleTypeDef hltdc;
extern UART_HandleTypeDef huart5;

/* --- Configuration --- */
// We define how many rows of buffer space we want.
// 40 rows at 240 width = 9,600 pixels. At 16-bit, that's ~19KB of RAM.
#define HMI_SHUTTLE_PIXELS (HMI_LCD_WIDTH * 80)

/* --- Private Function Prototypes --- */
static void hmi_disp_flush(void *p_disp_drv, int32_t x1, int32_t y1, int32_t x2, int32_t y2, void *p_color);
static void hmi_touch_read(void *p_indev_drv, void *p_data);
static uint32_t hmi_flash_read(uint8_t *p_buffer, uint32_t address, uint32_t size);
static atk_status_t hmi_uart_send(const uint8_t *data, size_t len);

/* --- External SDRAM Buffers --- */
/* We use the .sdram_data section defined in our Linker Script */
/* 240 * 160 * 2 bytes = 76,800 bytes per buffer. We have 8MB available! */
static uint16_t hmi_sdram_buf1[HMI_LCD_WIDTH * 160] __attribute__((section(".sdram_data")));
static uint16_t hmi_sdram_buf2[HMI_LCD_WIDTH * 160] __attribute__((section(".sdram_data")));

/* --- Hardware Interface Configuration --- */
static HMI_Hardware_Interface_t hmi_hw = {
	.screen_width = HMI_LCD_WIDTH,
	.screen_height = HMI_LCD_HEIGHT,
	.color_depth = (ART_ColorDepth_t)ATK_COLOR_DEPTH,

	/* Managed Memory Configuration */
	.disp_buffer_pix_count = (HMI_LCD_WIDTH * 160),
	.use_double_buffering = true, // We have enough SRAM on the F429 for two shuttle buffers

	.custom_buffer1 = hmi_sdram_buf1,
	.custom_buffer2 = hmi_sdram_buf2,

	.disp_flush_cb = hmi_disp_flush,
	.read_flash = hmi_flash_read,
	.read_input_cb = hmi_touch_read,
	.comm_send = hmi_uart_send,
	.comm_recv = NULL};

/**
 * @brief Renders the maintenance interface using 16-bit aligned BSP calls.
 */
static void HMI_EnterMaintenanceMode(void)
{
	/* Clear to professional Black */
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	/* Branding Header */
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_DisplayStringAt(0, 80, (uint8_t *)"ARTOK CORE", CENTER_MODE);
}

/* =========================================================================
   Public API Implementation (Defined in hmi.h)
   ========================================================================= */

void HMI_Init(void)
{
	hmiDma2dSemHandle = osSemaphoreCreate(osSemaphore(hmiDma2dSem), 1);
	osSemaphoreWait(hmiDma2dSemHandle, 0); // Start taken
	osThreadDef(hmiTask, StartHmiTask, HMI_TASK_PRIORITY, 0, HMI_TASK_STACK_SIZE);
	hmiTaskHandle = osThreadCreate(osThread(hmiTask), NULL);
}

/**
 * @brief  Prepares the physical display hardware for the HMI.
 * Ensures BSP defaults are corrected for Artok's requirements.
 */
static void HMI_PrepareHardware(void)
{
	/* 1. Initialize LCD */
	BSP_LCD_Init();

	/* 2. Setup Layer 0 */
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
	BSP_LCD_SelectLayer(0);

	hltdc.LayerCfg[0].PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
	HAL_LTDC_ConfigLayer(&hltdc, &hltdc.LayerCfg[0], 0);
	HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);

	/* 4. Prepare Canvas */
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_Clear(LCD_COLOR_BLUE);
	BSP_LCD_DisplayOn();

	/* 5. Touchscreen Init */
	BSP_TS_Init(HMI_LCD_WIDTH, HMI_LCD_HEIGHT);
}

void StartHmiTask(void const *argument)
{
	HMI_PrepareHardware();
	/* 2. Artok Initialization */
	ART_Init();
	ART_InitComm(&hmi_hw);
	ART_InitDisplay(&hmi_hw);
	ART_InitInput(&hmi_hw);

	/* 3. Start HMI from Flash Bank 2 */
	/**
	 * Use the macro directly.
	 * We cast it to a uint32_t because ART_StartHMI expects the address
	 * where the HMI data begins.
	 */
	uint32_t ui_addr = (uint32_t)UI_FLASH_START_ADDR;
	if (!ART_StartHMI((uint32_t)ui_addr, &hmi_hw))
	{
		HAL_GPIO_WritePin(GPIOG, LD4_Pin, GPIO_PIN_SET);
		for (;;)
			;
	}

	/* 4. Execution Loop */
	for (;;)
	{
		ART_MainLoop();
		osDelay(5);
	}
}

/**
 * @brief Bridge for UART callbacks to feed data into the runtime.
 */
void HMI_Feed_CommData(const uint8_t *data, uint16_t len)
{
	ART_FeedData(data, len);
}

/* =========================================================================
   Hardware Abstraction Implementations (Internal)
   ========================================================================= */

static uint32_t hmi_flash_read(uint8_t *p_buffer, uint32_t address, uint32_t size)
{
	// This allows the parser to read directly from the address we passed (ui_bin)
	memcpy(p_buffer, (void *)address, size);
	return size;
}

static void hmi_disp_flush(void *p_disp_drv, int32_t x1, int32_t y1, int32_t x2, int32_t y2, void *p_color)
{
    /* 1. Calculate Dimensions */
    int32_t width  = (x2 - x1 + 1);
    int32_t height = (y2 - y1 + 1);

    /* 2. Pointers */
    // Destination: The main FB in SDRAM (Layer 0)
    uint16_t *dst_fb = (uint16_t *)LCD_FRAME_BUFFER;
    // Source: The shuttle buffer provided by the Artok/LVGL runtime
    uint16_t *src_buf = (uint16_t *)p_color;

    /* 3. SAFETY CHECK: Bounds Validation */
    if (x2 >= HMI_LCD_WIDTH || y2 >= HMI_LCD_HEIGHT) {
        // If we hit this, the 'Large Panda' is being rendered with coordinates
        // that exceed your physical screen resolution!
        return;
    }

    /* 4. CPU-Based Row Copy (Bypassing DMA2D) */
    // We iterate through each row of the area being flushed.
    for (int32_t y = y1; y <= y2; y++)
    {
        // Calculate the starting position for this specific row in the Frame Buffer
        uint16_t *row_dst = &dst_fb[y * HMI_LCD_WIDTH + x1];

        // Copy the entire row of pixels (width * 2 bytes for RGB565)
        memcpy(row_dst, src_buf, width * 2);

        // Advance the source pointer by the width of the row we just copied
        src_buf += width;
    }

    /* 5. Finalize */
    // Since we are NOT using DMA2D_IT, we don't need to wait for a semaphore.
    // The CPU is finished once the loop is done.
    ART_FlushComplete();
}

static void hmi_touch_read(void *p_indev_drv, void *p_data)
{
	HMI_Touch_Data_t *data = (HMI_Touch_Data_t *)p_data;
	TS_StateTypeDef ts_state;

	BSP_TS_GetState(&ts_state);

	if (ts_state.TouchDetected)
	{
		// X usually matches left-to-right (0 to 240)
		data->x = (int16_t)ts_state.X;

		// Mirror the Y axis to match the LCD's Top-Left origin
		// This converts Bottom-Up coordinates to Top-Down
		data->y = (int16_t)(HMI_LCD_HEIGHT - ts_state.Y);

		data->pressed = true;
	}
	else
	{
		data->pressed = false;
	}
}

static atk_status_t hmi_uart_send(const uint8_t *data, size_t len)
{
	if (HAL_UART_Transmit(&huart5, (uint8_t *)data, (uint16_t)len, 100) == HAL_OK)
	{
		return ATK_STATUS_OK;
	}
	return ATK_STATUS_ERROR;
}

/**
 * @brief Resolves libc_nano warning for gettimeofday.
 */
int _gettimeofday(struct timeval *tv, void *tzvp)
{
	if (tv)
	{
		tv->tv_sec = HAL_GetTick() / 1000;
		tv->tv_usec = (HAL_GetTick() % 1000) * 1000;
	}
	return 0;
}
