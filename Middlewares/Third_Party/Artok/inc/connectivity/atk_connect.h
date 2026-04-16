#ifndef ATK_CONNECT_H
#define ATK_CONNECT_H

#include "atk_common.h"
#include <stddef.h>

/**
 * @brief Opaque handle representing a Master connection instance.
 */
typedef struct atk_connect_ctx atk_connect_handle_t;

/**
 * @brief Structure for messages received FROM the Slave (HMI -> Master).
 */
typedef struct {
    uint8_t  cid;         // Command/Response ID (e.g., ATK_RSP_ACK)
    uint16_t payload_len; // Length of the payload data
    const uint8_t* payload; 
} atk_message_t;

/**
 * @brief Callback function type for handling incoming data from the Slave.
 */
typedef void (*atk_rx_callback_t)(atk_connect_handle_t* handle, const atk_message_t* message);

// --- I. Lifecycle Management ---

/**
 * @brief Initializes a new Master connection context.
 * @param callback Function to handle responses/events from the Slave.
 * @return Pointer to the handle, or NULL on memory failure.
 */
atk_connect_handle_t* atk_connect_init(atk_rx_callback_t callback);

/**
 * @brief Registers the hardware transport layer for this specific handle.
 */
atk_status_t atk_connect_register_hal(
    atk_connect_handle_t* handle,
    atk_status_t (*send_fn)(const uint8_t* data, size_t len),
    size_t (*recv_fn)(uint8_t* buffer, size_t max_len)
);

// --- II. Master-to-Slave Commands (Binary Path) ---

/**
 * @brief Sends binary data to update a specific UI widget (CID: 0x20).
 */
atk_status_t atk_connect_set_data(
    atk_connect_handle_t* handle,
    uint16_t widget_id,
    atk_data_type_t type,
    const uint8_t* val_ptr,
    size_t val_len
);

/**
 * @brief Sends a command to switch the active screen (CID: 0x20).
 */
atk_status_t atk_connect_navigate(atk_connect_handle_t* handle, uint16_t screen_id);

// --- III. Master-to-Slave Commands (JSON Path) ---

/**
 * @brief Sends a JSON-formatted API command (CID: 0x10).
 * This is the primary tool for your React Terminal and flexible logic.
 * * @param json_str The UTF-8 JSON string.
 * @param len The length of the string.
 */
atk_status_t atk_connect_send_json(
    atk_connect_handle_t* handle, 
    const char* json_str, 
    size_t len
);

// --- IV. System Control ---

/**
 * @brief Sends a PING to the Slave to check connectivity (CID: 0x01).
 */
atk_status_t atk_connect_ping(atk_connect_handle_t* handle);

/**
 * @brief Drives the protocol state machine. 
 * Reads from HAL, verifies STX/ETX/CRC, and triggers the rx_callback.
 */
atk_status_t atk_connect_process(atk_connect_handle_t* handle);


/**
 * @brief Direct-feed process: Forwards raw bytes (e.g., from an ISR) to the state machine.
 */
void atk_connect_feed(atk_connect_handle_t* handle, const uint8_t* data, size_t len);


/**
 * @brief Frees all memory associated with the connection handle.
 */
void atk_connect_deinit(atk_connect_handle_t* handle);

#endif // ATK_CONNECT_H
