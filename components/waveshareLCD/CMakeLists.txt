idf_component_register(
    SRCS "lcdDisplay.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio
    REQUIRES esp_driver_ledc 
    REQUIRES esp_lcd_st7701 esp_lcd
    REQUIRES esp_lcd_panel_io_additions
    REQUIRES lvgl
    REQUIRES esp_timer
)


