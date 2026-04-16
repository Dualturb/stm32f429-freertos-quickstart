#ifndef ATK_COMMON_H
#define ATK_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Library-wide status and error codes.
 * Standardizes return values for both Master (React/Vite) and Client (STM32/WASM).
 */
enum atk_status_e {
    ATK_OK = 0,                 ///< Operation successful
    ATK_ERR_GENERIC,            ///< General failure
    ATK_ERR_HAL,
    ATK_ERR_INVALID_PARAM,      ///< NULL pointer or invalid length passed
    ATK_ERR_NOT_INITIALIZED,    ///< Context or HAL not registered
    ATK_ERR_BUFFER_OVERFLOW,    ///< Payload exceeds ARTOK_MAX_PAYLOAD_SIZE
    ATK_ERR_TX_FAILED,          ///< HAL layer failed to transmit data
    ATK_ERR_CRC_MISMATCH,       ///< Frame failed checksum validation
    ATK_ERR_PROTOCOL,           ///< Logic error (e.g., missing ETX or invalid CID)
    ATK_ERR_TIMEOUT,             ///< Operation timed out
};

/* * We wrap the typedef in a guard. If atk_runtime.h already defined it 
 * as int32_t, this prevents the conflict.
 */
#ifndef INC_ATK_RUNTIME_H_
typedef int32_t atk_status_t;
#endif

/**
 * @brief Data types supported for the BIN_API (CID: 0x20).
 * Used when sending raw structs to avoid JSON overhead for high-speed data.
 */
typedef enum {
    ATK_DATA_TYPE_INT32     = 0x01,  ///< 4-byte signed integer
    ATK_DATA_TYPE_STRING    = 0x02,  ///< ASCII string
    ATK_DATA_TYPE_FLOAT     = 0x03,  ///< 4-byte IEEE 754 float
    ATK_DATA_TYPE_BOOLEAN   = 0x04,  ///< 1-byte boolean (0 or 1)
    ATK_DATA_TYPE_RAW       = 0x05   ///< Binary blob / opaque data
} atk_data_type_t;

/**
 * @brief Command Handler Definitions
 */
typedef void (*atk_cmd_fn)(const char* json, uint16_t len);

struct atk_cmd_entry_s {
    const char* cmd_name;
    atk_cmd_fn  handler;
};

/* Only typedef if not already done in runtime.h */
#ifndef INC_ATK_RUNTIME_H_
typedef struct atk_cmd_entry_s atk_cmd_entry_t;
#endif

/**
 * @brief JSON Extraction Helpers (Exposed to Customer)
 */
bool atk_json_get_int(const char *json, uint16_t len, const char *key, int *out_val);
bool atk_json_get_str(const char *json, uint16_t len, const char *key, char *out_buf, size_t buf_size);
bool atk_json_get_bool(const char *json, uint16_t len, const char *key, bool *out_val);


#define ATK_MAX_PAYLOAD_SIZE          512
#define ATK_MAX_FRAME_SIZE            (ATK_MAX_PAYLOAD_SIZE + 7) 

// --- II. Unified Command IDs ---
#define ATK_CMD_PING                  0x01
#define ATK_CMD_JSON_API              0x10  // Payload: UTF-8 JSON String
#define ATK_CMD_BIN_API               0x20  // Payload: Raw C-Struct
#define ATK_RSP_ACK                   0x80
#define ATK_RSP_ERROR                 0x81

// --- III. Prototypes ---
uint16_t atk_protocol_calculate_crc(const uint8_t *data, size_t len);

size_t atk_protocol_make_frame(uint8_t *buffer, size_t buffer_size, 
                               uint8_t cid, const uint8_t *payload, 
                               size_t payload_len);

atk_status_t atk_protocol_parse_frame(const uint8_t *frame_ptr, size_t frame_len,
                                      uint8_t *out_cid, uint8_t *out_payload, 
                                      uint16_t *out_payload_len);

#endif // ATK_COMMON_H
