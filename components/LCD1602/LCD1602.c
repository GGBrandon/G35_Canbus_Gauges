#include "LCD1602.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define LCD_RS GPIO_NUM_9
#define LCD_E  GPIO_NUM_8

#define LCD_D4 GPIO_NUM_6
#define LCD_D5 GPIO_NUM_5
#define LCD_D6 GPIO_NUM_4
#define LCD_D7 GPIO_NUM_3

static void lcd_pulse_enable(void)
{
    gpio_set_level(LCD_E, 1);
    esp_rom_delay_us(1);

    gpio_set_level(LCD_E, 0);
    esp_rom_delay_us(50);
}

static void lcd_write_nibble(uint8_t nibble)
{
    gpio_set_level(LCD_D4, (nibble >> 0) & 1);
    gpio_set_level(LCD_D5, (nibble >> 1) & 1);
    gpio_set_level(LCD_D6, (nibble >> 2) & 1);
    gpio_set_level(LCD_D7, (nibble >> 3) & 1);

    lcd_pulse_enable();
}

static void lcd_send(uint8_t value, uint8_t rs)
{
    gpio_set_level(LCD_RS, rs);

    // High nibble
    lcd_write_nibble(value >> 4);

    // Low nibble
    lcd_write_nibble(value & 0x0F);
}

static void lcd_command(uint8_t command)
{
    lcd_send(command, 0);

    if (command == 0x01 || command == 0x02) {
        esp_rom_delay_us(2000);
    }
}

void lcd_write_char(char c)
{
    lcd_send((uint8_t)c, 1);
}

void lcd_print(const char *text)
{
    while (*text) {
        lcd_write_char(*text++);
    }
}

void lcd_clear(void)
{
    lcd_command(0x01);
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    uint8_t address;

    if (row == 0) {
        address = 0x00 + col;
    } else {
        address = 0x40 + col;
    }

    lcd_command(0x80 | address);
}

void lcd_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << LCD_RS) |
            (1ULL << LCD_E)  |
            (1ULL << LCD_D4) |
            (1ULL << LCD_D5) |
            (1ULL << LCD_D6) |
            (1ULL << LCD_D7),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    // Initial state
    gpio_set_level(LCD_RS, 0);
    gpio_set_level(LCD_E, 0);

    esp_rom_delay_us(50000);

    // HD44780 initialization sequence
    lcd_write_nibble(0x03);
    esp_rom_delay_us(5000);

    lcd_write_nibble(0x03);
    esp_rom_delay_us(150);

    lcd_write_nibble(0x03);
    esp_rom_delay_us(150);

    lcd_write_nibble(0x02);
    esp_rom_delay_us(150);

    // 4-bit mode, 2 lines, 5x8 font
    lcd_command(0x28);

    // Display off
    lcd_command(0x08);

    // Clear display
    lcd_command(0x01);

    // Entry mode: increment cursor
    lcd_command(0x06);

    // Display on, cursor off, blink off
    lcd_command(0x0C);
}