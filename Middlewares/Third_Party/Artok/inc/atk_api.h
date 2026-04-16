/**
 * @file atk_api.h
 * @brief Public API for controlling the Artok HMI from application code.
 */

#ifndef ATK_API_H
#define ATK_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/* --- Common Status Codes --- */
typedef enum {
    ATK_API_OK            = 0,
    ATK_API_ERR_NOT_FOUND = 1,  /* UUID not in registry */
    ATK_API_ERR_INVALID   = 2,  /* Invalid parameters or NULL pointers */
    ATK_API_ERR_TYPE      = 3   /* Widget exists but doesn't support the action */
} atk_api_status_t;


/**
 * @brief HMI System Performance and Memory Statistics.
 * Abstracted to prevent leaking LVGL types to high-level callers.
 */
typedef struct {
    size_t   mem_free;  /* Available heap in bytes */
    size_t   mem_used;  /* Allocated heap in bytes */
    uint8_t  mem_frag;  /* Fragmentation percentage (0-100) */
    uint16_t fps;       /* Current average frames per second */
} atk_system_stats_t;

/* =========================================================================
   Widget Property Setters
   ========================================================================= */

/**
 * @brief Sets the text property of a widget (Label, Button, Textarea).
 */
atk_api_status_t atk_api_set_text(const char* uuid, const char* text);

/**
 * @brief Sets a numeric value (Slider, Gauge, Progress bar, Arc, Checkbox).
 */
atk_api_status_t atk_api_set_value(const char* uuid, int32_t value);

/**
 * @brief Configures the min and max limits for numeric widgets.
 */
atk_api_status_t atk_api_set_range(const char* uuid, int32_t min, int32_t max);

/**
 * @brief Toggles widget visibility (Hidden/Shown).
 */
atk_api_status_t atk_api_set_visible(const char* uuid, bool visible);

/**
 * @brief Toggles interactivity (Gray-out effect and touch lock).
 */
atk_api_status_t atk_api_set_enabled(const char* uuid, bool enabled);

/**
 * @brief Sets a semantic color (Text for labels, Background for containers).
 */
atk_api_status_t atk_api_set_color(const char* uuid, uint32_t hex_color);

/* =========================================================================
   Graphics & Asset Control
   ========================================================================= */

/**
 * @brief Swaps the image source of an Image or ImageButton widget.
 * @param uuid The target widget UUID.
 * @param img_uuid The Resource UUID of the image in flash.
 */
atk_api_status_t atk_api_set_image(const char* uuid, const char* img_uuid);

/**
 * @brief Swaps the animation sequence of an Animated Image widget.
 * @param uuid The target widget UUID.
 * @param seq_uuid The Resource UUID of the animation sequence in flash.
 */
atk_api_status_t atk_api_set_animation(const char* uuid, const char* seq_uuid);

/**
 * @brief Rotates an image widget.
 * @param angle Angle in 0.1 degree units (0 to 3600).
 */
atk_api_status_t atk_api_set_rotation(const char* uuid, int16_t angle);

/**
 * @brief Updates the structural geometry of a Meter or Arc.
 * @param angle Meter: Total sweep angle (e.g. 270). Arc: Background start angle.
 * @param rotation Meter: Start rotation offset. Arc: Background end angle.
 */
atk_api_status_t atk_api_set_geometry(const char* uuid, uint16_t angle, uint16_t rotation);

/* =========================================================================
   Widget Property Getters
   ========================================================================= */

/**
 * @brief Retrieves the current text of a widget.
 */
atk_api_status_t atk_api_get_text(const char* uuid, char* out_buf, uint32_t buf_size);

/**
 * @brief Retrieves the current numeric value/state of a widget.
 */
atk_api_status_t atk_api_get_value(const char* uuid, int32_t* out_val);

/**
 * @brief Retrieves the min and max limits of a numeric widget.
 */
atk_api_status_t atk_api_get_range(const char* uuid, int32_t* out_min, int32_t* out_max);

void atk_api_get_stats(atk_system_stats_t* out_stats);

void atk_api_clear_display(void);

#endif /* ATK_API_H */