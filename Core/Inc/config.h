#ifndef INC_CONFIG_H
#define INC_CONFIG_H

/* --- 1. Flash Geometry --- */
/* This matches your Linker Script UI_FLASH origin */
#define UI_FLASH_START_ADDR     0x08100000
/* 1MB total space */
#define UI_FLASH_MAX_SIZE       (1024 * 1024)

/* --- 2. Runtime Memory --- */
/* Address of the SDRAM region for framebuffers and dynamic UI elements */
#define ATK_RAM_BASE            0xD0000000
#define ATK_RAM_SIZE            (8 * 1024 * 1024)

/* --- Display Constants (STM32F429 Discovery) --- */
#define HMI_LCD_WIDTH        240
#define HMI_LCD_HEIGHT       320

/**
 * @brief The address of the SDRAM Framebuffer.
 * Matches your BSP configuration.
 */
#define LCD_FRAME_BUFFER     (0xD0000000)

#endif /* INC_CONFIG_H */
