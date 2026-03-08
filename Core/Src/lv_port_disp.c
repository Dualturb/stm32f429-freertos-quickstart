#include "lv_port_disp.h"
#include "lvgl/lvgl.h"
#include "main.h"

/* Parameters for STM32F429I-DISC1 */
#define LCD_FRAME_BUFFER 0xD0000000
#define MY_DISP_HOR_RES  240
#define MY_DISP_VER_RES  320

/* External handle for the DMA2D initialized in main.c */
extern DMA2D_HandleTypeDef hdma2d;

/* Forward Declaration */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

void lv_port_disp_init(void) {
    /* 1. Static draw buffer for LVGL to paint into (Internal RAM) */
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[MY_DISP_HOR_RES * 40];

    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, MY_DISP_HOR_RES * 40);

    /* 2. Initialize the display driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {

    uint32_t width  = lv_area_get_width(area);
    uint32_t height = lv_area_get_height(area);

    /* 1. Calculate destination address in SDRAM */
    // area->y1 * 240 + area->x1 gives the pixel offset, then we cast to uint32_t for DMA2D
    uint32_t destination = LCD_FRAME_BUFFER + (area->y1 * MY_DISP_HOR_RES + area->x1) * 2;

    /* 2. Configure DMA2D */
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_M2M; // Memory to Memory
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = MY_DISP_HOR_RES - width;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK) {
        /* Layer 1 (Foreground) Configuration */
        DMA2D_LayerCfgTypeDef layer_cfg;
        layer_cfg.InputOffset = 0;
        layer_cfg.InputColorMode = DMA2D_INPUT_RGB565;
        layer_cfg.AlphaMode = DMA2D_NO_MODIF_ALPHA;
        layer_cfg.InputAlpha = 0xFF;

        HAL_DMA2D_ConfigLayer(&hdma2d, 1);

        /* 3. Start Transfer */
        // Note: Using HAL_DMA2D_Start involves Layer 1 by default in M2M mode
        if (HAL_DMA2D_Start(&hdma2d, (uint32_t)color_p, destination, width, height) == HAL_OK) {
            HAL_DMA2D_PollForTransfer(&hdma2d, 10);
        }
    }

    /* 4. IMPORTANT: Signal LVGL completion */
    lv_disp_flush_ready(disp_drv);
}
