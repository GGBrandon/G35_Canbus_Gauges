#ifndef LCDDISPLAY_H
#define LCDDISPLAY_H

#include <stdint.h>

// init display
void lcd_init(void);
void lcd_lvgl_init(void);

// fill background
void lcd_fill(uint16_t color);
void lcd_backlight_init(uint16_t duty);


#endif