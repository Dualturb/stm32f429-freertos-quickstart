#include "lv_port_indev.h"

static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data);

/* Input device handle */
lv_indev_t * indev_touchpad;

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /* 1. Initialize the Hardware (BSP) */
    touchpad_init();

    /* 2. Register the driver in LVGL */
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;

    /* Register the driver and save the created input device object */
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

static void touchpad_init(void)
{
    /* The STM32F429I-DISC1 display is 240x320 */
    if (BSP_TS_Init(240, 320) != TS_OK) {
        /* Error handling: You could toggle an LED here to signal I2C failure */
    }
}

static void touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
    static TS_StateTypeDef TS_State;

    /* Get the current state from the BSP */
    BSP_TS_GetState(&TS_State);

    if(TS_State.TouchDetected) {
        data->state = LV_INDEV_STATE_PR;

        /* IMPORTANT: Coordinates might need swapping based on your screen rotation */
        /* For the default orientation: */
        data->point.x = TS_State.X;
        data->point.y = TS_State.Y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
