#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include <config.h>
#include "cmsis_os.h"
#include <stdint.h>

/* --- Public API --- */

/**
 * @brief Initializes the Flash task and Ring Buffer.
 * Should be called in main.c before the RTOS scheduler starts.
 */
void FLASH_Init(void);

/**
 * @brief The FreeRTOS task body for handling Flash operations.
 */
void StartFlashTask(void const * argument);

/**
 * @brief Feeds a byte from the UART Interrupt into the software ring buffer.
 * Safe to call from ISR.
 * @param byte The received character from USART1.
 */
void FLASH_Feed_Byte(uint8_t byte);

// Tell main.c that the byte variable lives in flash.c
extern uint8_t uart_rx_byte;

#endif /* INC_FLASH_H_ */
