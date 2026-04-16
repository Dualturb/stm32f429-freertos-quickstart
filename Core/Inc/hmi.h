/**
 * @file hmi.h
 * @brief Application-level HMI task management for the Artok ecosystem.
 */

#ifndef INC_HMI_H_
#define INC_HMI_H_

#include <config.h>
#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

/* --- HMI Task Configuration --- */

/**
 * @brief Stack size for the HMI Task (Words).
 * 8KB (2048 words) is recommended if using heavy Lua scripting or
 * complex fragment nesting.
 */
#define HMI_TASK_STACK_SIZE  (1024*8)

/**
 * @brief Priority for the UI engine.
 * Set slightly higher than background comms, lower than safety-critical tasks.
 */
#define HMI_TASK_PRIORITY    osPriorityNormal

/* --- Public API --- */

/**
 * @brief Initializes the HMI resources and creates the FreeRTOS thread.
 * Call this in main.c after HAL/Peripheral inits but before osKernelStart().
 */
void HMI_Init(void);

/**
 * @brief The main HMI Task function.
 */
void StartHmiTask(void const * argument);

/**
 * @brief Global access to the HMI Thread ID.
 */
extern osThreadId hmiTaskHandle;

/* --- Hardware Notification Hooks --- */

/**
 * @brief Signals the HMI engine that a display transfer is complete.
 */
void HMI_SignalRenderDone(void);

/**
 * @brief Feeds raw communication data into the UI engine Southbound pipe.
 * @param data Pointer to the received byte(s).
 * @param len  Length of the data.
 */
void HMI_Feed_CommData(const uint8_t* data, uint16_t len);


#endif /* INC_HMI_H_ */
