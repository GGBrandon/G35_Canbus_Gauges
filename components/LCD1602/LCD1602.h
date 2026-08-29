#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);
void lcd_write_char(char c);

#endif