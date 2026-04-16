/**
 * @file atk_runtime.h
 * @brief Defines the core API and hardware abstraction layer for the ARTOK HMI runtime.
 *
 * This header provides the necessary structures and functions to initialize,
 * run, and manage the HMI, bridging the gap between the generated UI
 * and the target hardware.
 */

#ifndef INC_ATK_RUNTIME_H_
#define INC_ATK_RUNTIME_H_

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int32_t atk_status_t;
#define ATK_STATUS_OK 0
#define ATK_STATUS_ERROR -1

struct atk_cmd_entry_s;
typedef struct atk_cmd_entry_s atk_cmd_entry_t;

/** * @brief Definition for the custom command callback.
 * @param json The JSON payload string (null-terminated).
 * @param len  The length of the JSON string.
 */
typedef void (*atk_cmd_handler_t)(const char *json, uint16_t len);

/**
 * @brief HMI color depth enumeration.
 * Allows the library to adapt its internal rendering or conversion.
 */
typedef enum
{
    ART_COLOR_DEPTH_1 = 1,
    ART_COLOR_DEPTH_8 = 8,
    ART_COLOR_DEPTH_16 = 16,
    ART_COLOR_DEPTH_32 = 32
} ART_ColorDepth_t;

/**
 * @brief Generic data structure for touch input.
 */
typedef struct
{
    int16_t x;
    int16_t y;
    bool pressed;
} HMI_Touch_Data_t;

/**
 * @brief Represents the hardware abstraction layer (HAL) interface.
 *
 * This struct provides function pointers and data addresses that the HMI runtime
 * will use to interact with the target hardware's peripherals.
 */
typedef struct
{
    /* --- Display Geometry (Dynamic Configuration) --- */
    uint16_t screen_width;
    uint16_t screen_height;
    ART_ColorDepth_t color_depth;

    /* --- Managed Memory Configuration --- */
    /** * @brief Number of pixels to allocate for the draw buffers.
     * The runtime will calculate the byte size based on color_depth.
     */
    uint32_t disp_buffer_pix_count;

    /** @brief If true, the runtime allocates two buffers for flicker-free DMA. */
    bool use_double_buffering;


    /* --- Manual Memory Management (Optional) --- */
    /** * @brief Custom Draw Buffers.
     * If these are non-NULL, the runtime will skip malloc() and use these 
     * addresses directly. Ideal for placing buffers in External SDRAM.
     */
    void *custom_buffer1;
    void *custom_buffer2;

    /** * @brief The display flush callback.
     * @param p_color Opaque pointer to the rendered pixel data (16 or 32-bit).
     */
    void (*disp_flush_cb)(void *p_disp_drv, int32_t area_x1, int32_t area_y1,
                          int32_t area_x2, int32_t area_y2, const void *p_color);

    /* --- Hardware Acceleration Hooks (Optional) --- */
    /* By leaving these NULL, the runtime stays 'Pure' and uses Software rendering.
     * By providing them, the Firmware enables 'Industrial Grade' performance. */

    /** * GPU Fill: Accelerates large area clearing/coloring (e.g., DMA2D Fill)
     * Destination is assumed to be the Physical Framebuffer.
     * * @param x      Starting X coordinate on screen.
     * @param y      Starting Y coordinate on screen.
     * @param w      Width of the area to fill.
     * @param h      Height of the area to fill.
     * @param color  The color value (formatted for the target depth).
     */
    void (*gpu_fill_cb)(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

    /** * GPU Blit: Accelerates image/buffer copying (e.g., DMA2D M2M)
     * The destination is assumed to be the Physical Framebuffer managed by the HAL.
     * * @param src_buf    The source buffer (rendered by LVGL).
     * @param src_width  The stride/width of the source buffer.
     * @param w          Width of the area to blit.
     * @param h          Height of the area to blit.
     * @param x          Destination X-coordinate on screen.
     * @param y          Destination Y-coordinate on screen.
     */
    void (*gpu_blit_cb)(const void *src_buf, uint32_t src_width, int32_t x, int32_t y, int32_t w, int32_t h);

    /* --- Peripheral Callbacks --- */
    uint32_t (*read_flash)(uint8_t *p_buffer, uint32_t address, uint32_t size);
    void (*read_input_cb)(void *p_indev_drv, void *p_data);
    atk_status_t (*comm_send)(const uint8_t *data, size_t len);
    size_t (*comm_recv)(uint8_t *buffer, size_t max_len);
} HMI_Hardware_Interface_t;

/**********************
 * PUBLIC API
 **********************/

/**
 * @brief TIER 1: Initialize the core UI engine.
 * Wraps lv_init(). Call this once at system startup.
 */
void ART_Init(void);

void ART_InitComm(const HMI_Hardware_Interface_t *p_hw);

/**
 * @brief TIER 2: Initialize the Display driver.
 * Registers the display flush callback and configures drawing buffers.
 * @param p_hw Pointer to the HAL interface containing display data.
 */
void ART_InitDisplay(const HMI_Hardware_Interface_t *p_hw);

/**
 * @brief Returns a pointer to the internally managed display buffers.
 * @param index 0 for the primary buffer, 1 for the secondary (if double buffered).
 * @return void* Pointer to the buffer, or NULL if not allocated.
 */
void *ART_GetBuffer(uint8_t index);

/**
 * @brief Returns the total size in bytes of a single display buffer.
 */
uint32_t ART_GetBufferSize(void);

/**
 * @brief TIER 3: Initialize the Input driver (Optional).
 * Registers the touch/input reading callback.
 * @param p_hw Pointer to the HAL interface containing input data.
 */
void ART_InitInput(const HMI_Hardware_Interface_t *p_hw);

/**
 * @brief Verifies the UI binary header and checksum.
 * Call this before ART_StartHMI to ensure flash integrity.
 */
atk_status_t ART_ValidateUI(uint32_t ui_binary_addr, const HMI_Hardware_Interface_t *p_hw);

/**
 * @brief Renders a high-level system message using the agnostic display pipeline.
 * Useful for boot screens or fatal error reporting.
 * @param title Header text.
 * @param message Body text.
 * @param is_error If true, uses a red background; otherwise deep blue.
 */
void ART_ShowCoreUI(const char *title, const char *message, bool is_error);

/**
 * @brief TIER 4: Start the HMI application logic.
 * Parses the UI binary, initializes Lua, and loads the entry screen.
 * @param ui_binary_addr Flash address of the UI binary.
 * @param p_hw Pointer to the HAL interface (required for flash access).
 * @return true if successful.
 */
bool ART_StartHMI(uint32_t ui_binary_addr, const HMI_Hardware_Interface_t *p_hw);

/**
 * @brief Execution loop. Must be called periodically in the main loop.
 */
void ART_MainLoop(void);

/**
 * @brief Increments the internal tick counter.
 * Call this every 1ms from a system timer interrupt.
 */
void ART_IncTick(uint32_t ms);

/**
 * @brief Signals that a DMA display transfer is complete.
 * Must be called from the display driver's "Transfer Complete" interrupt.
 */
void ART_FlushComplete(void);

/**
 * @brief Feed raw data into the Southbound pipe.
 * Usually called from a UART/DMA RX Interrupt.
 */
void ART_FeedData(const uint8_t *data, size_t len);

/**
 * @brief De-initializes the runtime and releases all resources.
 */
void ART_Cleanup(void);

/**
 * @brief Registers a custom command handler.
 * @param cmd_name The trigger string (e.g., "reboot").
 * @param handler  The function to call.
 */
void ART_RegisterCustomCmd(const char *cmd_name, atk_cmd_handler_t handler);

typedef void (*art_probe_cb_t)(const char *key, int32_t value, void *user_data);
/**
 * @brief Probes an HMI binary to extract hardware requirements.
 * This keeps the Engine and Runtime strictly decoupled.
 */
atk_status_t ART_Probe(const uint8_t *bin_ptr, size_t bin_size, art_probe_cb_t cb, void *user_data);

/**
 * @brief 32-bit Output Format Selectors
 */
typedef enum
{
    ART_FORMAT_RGBA8888, // Standard Web/Android (R-G-B-A)
    ART_FORMAT_BGRA8888, // Standard Windows/DirectX (B-G-R-A)
    ART_FORMAT_ARGB8888  // Standard MacOS/iOS (A-R-G-B)
} ART_PixelFormat_t;

/**
 * @brief Converts native pixels to a unified 32-bit format.
 * * @param p_src     The native buffer (1, 8, or 16-bit).
 * @param p_dst     The 32-bit destination buffer.
 * @param pix_count Number of pixels to process.
 * @param format    The target 32-bit bitmask (RGBA, BGRA, etc).
 */
void ART_NativeTo32(const void *p_src, uint32_t *p_dst, uint32_t pix_count, ART_PixelFormat_t format);

/**
 * @brief Public accessor for the internal hardware stage.
 * * This returns an opaque pointer to the `atk_stage_t` singleton.
 * Plug-ins like artok-editor can cast this back to a local `atk_stage_t`
 * definition to "hijack" the display and input drivers without the
 * Runtime needing to expose LVGL headers in its public `atk_runtime.h`.
 * * @return Opaque pointer to the persistent stage.
 */
void *ART_GetStageHandle(void);
#endif /* INC_ATK_RUNTIME_H_ */
