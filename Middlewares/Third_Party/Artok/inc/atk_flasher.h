/*
 * atk_flasher.h
 *
 * This header defines the public API and the hardware-agnostic interface
 * for the artok-flasher library.
 */

#ifndef ATK_FLASHER_H
#define ATK_FLASHER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @struct Atk_FlasherInterface_t
 * @brief Defines the hardware-agnostic interface for the artok-flasher library.
 * This struct contains function pointers for all required hardware operations.
 * The host firmware must provide the concrete implementations for its specific
 * hardware (e.g., UART, SPI, I2C, etc.).
 */
typedef struct {
    // Pre-initialization callback, called once before any operations.
    void (*preinit)(void);
    
    // Protocol callback for sending a response acknowledgment bytes back to the host.
    void (*send_response_byte)(uint8_t byte);
    
    // Generic device control for initiating a transaction.
    void (*device_select)(void);
    void (*device_deselect)(void);
    
    // Generic data transfer functions.
    // Use the function that best fits the communication pattern.
    void (*transmit_data)(const uint8_t* tx_data, uint16_t size);
    void (*receive_data)(uint8_t* rx_data, uint16_t size);
    void (*transmit_receive)(const uint8_t* tx_data, uint8_t* rx_data, uint16_t size);

    // Timing callback.
    void (*delay_ms)(uint32_t ms);
} Atk_FlasherInterface_t;

/**
 * @brief Abstract flash driver interface for high-level operations.
 *
 * This struct defines a set of function pointers for flash chip-specific
 * operations like reading, writing, and erasing. This is the "what"
 * of the protocol.
 */
typedef struct {
    // Initializes the flash chip.
    bool (*init)(const Atk_FlasherInterface_t* interface);
    // Reads data from the flash chip.
    bool (*read)(const Atk_FlasherInterface_t* interface, uint32_t address, uint8_t* data, uint32_t size);
    // Writes data to the flash chip.
    bool (*write)(const Atk_FlasherInterface_t* interface, uint32_t address, const uint8_t* data, uint32_t size);
    // Erases a sector of the flash chip.
    bool (*erase_sector)(const Atk_FlasherInterface_t* interface, uint32_t address);
    // Erases the entire flash chip.
    bool (*erase_chip)(const Atk_FlasherInterface_t* interface);
    // Gets the total size of the flash chip in bytes.
    uint32_t (*get_size)(const Atk_FlasherInterface_t* interface);
} Atk_FlasherDriver_t;

// --- Public API Functions ---
/**
 * @brief External declaration for the built-in W25Qxx driver.
 * This allows the core library to reference it when a driver is not provided.
 */
extern const Atk_FlasherDriver_t w25qxx_driver;

/**
 * @brief Initializes the artok-flasher library and registers hardware callbacks.
 * This is the first function a host application should call.
 * @param interface Pointer to the struct of hardware-specific function pointers.
 */
void flasher_init(const Atk_FlasherInterface_t* interface, const Atk_FlasherDriver_t* driver);

/**
 * @brief Feeds a single byte into the flasher's internal ring buffer for processing.
 * The host's protocol handler (e.g., a UART RX ISR) must call this function
 * for every byte received from the host computer.
 * @param byte The received byte to process.
 */
void flasher_process_byte(uint8_t byte);

/**
 * @brief Main processing function for the flasher state machine.
 * This should be called periodically from the host's main loop to handle
 * the flash programming protocol and operations.
 */
void flasher_process(void);

uint8_t flasher_get_state(void);

#endif /* ATK_FLASHER_H */
