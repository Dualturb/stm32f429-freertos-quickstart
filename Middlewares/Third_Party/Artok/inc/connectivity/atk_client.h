#ifndef ATK_CLIENT_H
#define ATK_CLIENT_H

#include "atk_common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- I. Lifecycle & Configuration --- */

/**
 * @brief Initialize the client with a tiered command structure.
 */
atk_status_t atk_client_init_ext(const atk_cmd_entry_t *custom_table, size_t custom_size,
                                const atk_cmd_entry_t *builtin_table, size_t builtin_size);

/**
 * @brief Simple initialization for standard foundation commands.
 */
atk_status_t atk_client_init(const atk_cmd_entry_t *table, size_t table_size);

/**
 * @brief Register Hardware Abstraction Layer (HAL) functions.
 */
atk_status_t atk_client_register_hal(
    atk_status_t (*send_fn)(const uint8_t* data, size_t len),
    size_t (*recv_fn)(uint8_t* buffer, size_t max_len)
);

/**
 * @brief Dynamically adds a custom command to the internal library-owned registry.
 * Eliminates the need for the application to manage command-table memory.
 */
atk_status_t atk_client_add_custom_cmd(const char* name, atk_cmd_fn handler);

/* --- II. Runtime Processing --- */

/**
 * @brief Processes the HAL receive buffer and state machine. 
 * Should be called frequently in the main system loop.
 */
atk_status_t atk_client_process(void);

/**
 * @brief Manually feeds raw bytes (e.g., from an interrupt or DMA) 
 * into the protocol parser.
 */
void atk_client_feed(const uint8_t* data, size_t len);

/**
 * @brief Low-level byte parser for the protocol state machine.
 */
void atk_client_parse_byte(uint8_t byte);

/* --- III. Upstream Communication (Client -> Master) --- */

/**
 * @brief Sends a generic event or binary data update to the Master.
 */
atk_status_t atk_client_send_event(uint8_t cid, const uint8_t* payload, size_t len);

/**
 * @brief Sends a Success Acknowledge (ACK) to the Master (CID: 0x80).
 */
atk_status_t atk_client_send_ack(void);

/**
 * @brief Sends a Protocol/Logic Error to the Master (CID: 0x81).
 * @param error_code Internal ATK error ID.
 */
atk_status_t atk_client_send_error(uint8_t error_code);

#ifdef __cplusplus
}
#endif

#endif // ATK_CLIENT_H
