#ifndef LCDCONFIG_H
#define LCDCONFIG_H

#include "driver/gpio.h"

//=======================================================
// Specifications from official Waveshare LVGL V9 example
//=======================================================


//Pin Defs
#define  LCD_PWM_MODE_0   (0xff-0)
#define  LCD_PWM_MODE_25  (0xff-25)
#define  LCD_PWM_MODE_50  (0xff-50)
#define  LCD_PWM_MODE_75  (0xff-75)
#define  LCD_PWM_MODE_100 (0xff-100)
#define  LCD_PWM_MODE_125 (0xff-125)
#define  LCD_PWM_MODE_150 (0xff-150)
#define  LCD_PWM_MODE_175 (0xff-175)
#define  LCD_PWM_MODE_200 (0xff-200)
#define  LCD_PWM_MODE_225 (0xff-225)
#define  LCD_PWM_MODE_255 (0xff-255)


// 3-Wire SPI pins (
#define LCD_CS  0   // chip Select
#define LCD_SCL 2   // SPI Clock
#define LCD_SDA 1   // SPI Data
#define LCD_RST 16  // reset


#define LCD_DE    40  
#define LCD_VSYNC 39  
#define LCD_HSYNC 38  
#define LCD_PCLK  41  


#define LCD_R0 17
#define LCD_R1 46
#define LCD_R2 3
#define LCD_R3 8
#define LCD_R4 18


#define LCD_G0 14
#define LCD_G1 13
#define LCD_G2 12
#define LCD_G3 11
#define LCD_G4 10
#define LCD_G5 9


#define LCD_B0 21
#define LCD_B1 5
#define LCD_B2 45
#define LCD_B3 48
#define LCD_B4 47

// Backlight Pin
#define LCD_BL 6  // 0=bright, 255=off


#define LCD_H_RES 320  
#define LCD_V_RES 820  
#define LCD_PIXEL_CLOCK_HZ (18 * 1000 * 1000)  // 18 MHz pixel clock

// preset color values I chose

/* 
* These are for 16-Bit RGB
#define COLOR_BLACK   0x0000  
#define COLOR_WHITE   0xFFFF  
#define COLOR_RED     0xF800  
#define COLOR_GREEN   0x07E0  
#define COLOR_BLUE    0x001F  
#define COLOR_MAGENTA 0xF81F  
*/

// These are hex for LVGL
#define COLOR_BLACK    0x000000
#define COLOR_WHITE    0xFFFFFF
#define COLOR_RED      0xFF0000
#define COLOR_GREEN    0x00FF00
#define COLOR_BLUE     0x0000FF
#define COLOR_MAGENTA  0xFF00FF

// LVGL
// taken from offical Waveshare example
#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     5



#endif // LCD_CONFIG_H