#include "lcdDisplay.h"
#include "lcdConfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_heap_caps.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"

#include "lvgl.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

#include "driver/ledc.h"


esp_lcd_panel_handle_t panel_handle = NULL;  // RGB panel handle

// lvgl flush
static SemaphoreHandle_t lvgl_mux = NULL;
static SemaphoreHandle_t flush_done_semaphore = NULL;

// from waveshare demo
static const st7701_lcd_init_cmd_t lcd_init_cmds[] =
{
    //   cmd   data        data_size  delay_ms 1
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x13 },5,0 },
    { 0xEF,(uint8_t[]) { 0x08 },1,0 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x10 },5,0 },
    { 0xC0,(uint8_t[]) { 0xE5,0x02 },2,0 },
    { 0xC1,(uint8_t[]) { 0x15,0x0A },2,0 },
    { 0xC2,(uint8_t[]) { 0x07,0x02 },2,0 },
    { 0xCC,(uint8_t[]) { 0x10 },1,0 },
    { 0xB0,(uint8_t[]) { 0x00,0x08,0x51,0x0D,0xCE,0x06,0x00,0x08,0x08,0x24,0x05,0xD0,0x0F,0x6F,0x36,0x1F },16,0 },
    { 0xB1,(uint8_t[]) { 0x00,0x10,0x4F,0x0C,0x11,0x05,0x00,0x07,0x07,0x18,0x02,0xD3,0x11,0x6E,0x34,0x1F },16,0 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x11 },5,0 },
    { 0xB0,(uint8_t[]) { 0x4D },1,0 },
    { 0xB1,(uint8_t[]) { 0x37 },1,0 },
    { 0xB2,(uint8_t[]) { 0x87 },1,0 },
    { 0xB3,(uint8_t[]) { 0x80 },1,0 },
    { 0xB5,(uint8_t[]) { 0x4A },1,0 },
    { 0xB7,(uint8_t[]) { 0x85 },1,0 },
    { 0xB8,(uint8_t[]) { 0x21 },1,0 },
    { 0xB9,(uint8_t[]) { 0x00,0x13 },2,0 },
    { 0xC0,(uint8_t[]) { 0x09 },1,0 },
    { 0xC1,(uint8_t[]) { 0x78 },1,0 },
    { 0xC2,(uint8_t[]) { 0x78 },1,0 },
    { 0xD0,(uint8_t[]) { 0x88 },1,0 },
    { 0xE0,(uint8_t[]) { 0x80,0x00,0x02 },3,100 },
    { 0xE1,(uint8_t[]) { 0x0F,0xA0,0x00,0x00,0x10,0xA0,0x00,0x00,0x00,0x60,0x60 },11,0 },
    { 0xE2,(uint8_t[]) { 0x30,0x30,0x60,0x60,0x45,0xA0,0x00,0x00,0x46,0xA0,0x00,0x00,0x00 },13,0 },
    { 0xE3,(uint8_t[]) { 0x00,0x00,0x33,0x33 },4,0 },
    { 0xE4,(uint8_t[]) { 0x44,0x44 },2,0 },
    { 0xE5,(uint8_t[]) { 0x0F,0x4A,0xA0,0xA0,0x11,0x4A,0xA0,0xA0,0x13,0x4A,0xA0,0xA0,0x15,0x4A,0xA0,0xA0 },16,0 },
    { 0xE6,(uint8_t[]) { 0x00,0x00,0x33,0x33 },4,0 },
    { 0xE7,(uint8_t[]) { 0x44,0x44 },2,0 },
    { 0xE8,(uint8_t[]) { 0x10,0x4A,0xA0,0xA0,0x12,0x4A,0xA0,0xA0,0x14,0x4A,0xA0,0xA0,0x16,0x4A,0xA0,0xA0 },16,0 },
    { 0xEB,(uint8_t[]) { 0x02,0x00,0x4E,0x4E,0xEE,0x44,0x00 },7,0 },
    { 0xED,(uint8_t[]) { 0xFF,0xFF,0x04,0x56,0x72,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x27,0x65,0x40,0xFF,0xFF },16,0 },
    { 0xEF,(uint8_t[]) { 0x08,0x08,0x08,0x40,0x3F,0x64 },6,0 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x13 },5,0 },
    { 0xE8,(uint8_t[]) { 0x00,0x0E },2,0 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x00 },5,0 },
    { 0x11,(uint8_t[]) { 0x00 },0,120 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x13 },5,0 },
    { 0xE8,(uint8_t[]) { 0x00,0x0C },2,10 },
    { 0xE8,(uint8_t[]) { 0x00,0x00 },2,0 },
    { 0xFF,(uint8_t[]) { 0x77,0x01,0x00,0x00,0x00 },5,0 },
    { 0x3A,(uint8_t[]) { 0x55 },1,0 },
    { 0x36,(uint8_t[]) { 0x00 },1,0 },
    { 0x35,(uint8_t[]) { 0x00 },1,0 },
    { 0x29,(uint8_t[]) { 0x00 },0,20 },
};


static bool rgb_panel_frame_done(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_data_t* edata,
    void* user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;

    xSemaphoreGiveFromISR(
        flush_done_semaphore,
        &high_task_awoken
    );

    return high_task_awoken == pdTRUE;
}

// RGB init
static void rgb_panel_init(void) {
    printf("Initializing ST7701...\n");

    // 3-wire SPI configuration for ST7701 initialization
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = LCD_CS,

        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = LCD_SCL,

        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = LCD_SDA,

        .io_expander = NULL,
    };

    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);

    esp_lcd_panel_io_handle_t io_handle = NULL;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_3wire_spi(
            &io_config,
            &io_handle
        )
    );

    printf("ST7701 SPI initialized\n");


    // RGB parallel configuration
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,

        .data_width = 16,

        .num_fbs = 1,

        .bounce_buffer_size_px = 10 * LCD_H_RES,

        .de_gpio_num = LCD_DE,
        .pclk_gpio_num = LCD_PCLK,
        .vsync_gpio_num = LCD_VSYNC,
        .hsync_gpio_num = LCD_HSYNC,

        .disp_gpio_num = -1,

        .flags = {
            .fb_in_psram = true,
        },

        // IMPORTANT:
        // The Waveshare wiring is B-G-R.
        .data_gpio_nums = {

            LCD_B0,
            LCD_B1,
            LCD_B2,
            LCD_B3,
            LCD_B4,


            LCD_G0,
            LCD_G1,
            LCD_G2,
            LCD_G3,
            LCD_G4,
            LCD_G5,


            LCD_R0,
            LCD_R1,
            LCD_R2,
            LCD_R3,
            LCD_R4,
        },

        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,

            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,

            .hsync_back_porch = 30,
            .hsync_front_porch = 30,
            .hsync_pulse_width = 6,

            .vsync_back_porch = 20,
            .vsync_front_porch = 20,
            .vsync_pulse_width = 40,
        },
    };


    // ST7701 vendor configuration
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,

        .init_cmds = lcd_init_cmds,

        .init_cmds_size =
            sizeof(lcd_init_cmds) /
            sizeof(st7701_lcd_init_cmd_t),

        .flags = {
            .mirror_by_cmd = 1,
            .enable_io_multiplex = 0,
        },
    };

    // ST7701 panel configuration
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,

        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,

        .bits_per_pixel = 16,

        .vendor_config = &vendor_config,
    };


    // Create ST7701 panel
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_st7701(
            io_handle,
            &panel_config,
            &panel_handle
        )
    );

    printf("ST7701 panel created\n");

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
    .on_color_trans_done = rgb_panel_frame_done,
    };

    ESP_ERROR_CHECK(
        esp_lcd_rgb_panel_register_event_callbacks(
            panel_handle,
            &cbs,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_lcd_panel_reset(panel_handle)
    );

    printf("ST7701 reset\n");

    ESP_ERROR_CHECK(
        esp_lcd_panel_init(panel_handle)
    );

    ESP_ERROR_CHECK(
        esp_lcd_panel_disp_on_off(panel_handle, true)
    );


    printf("ST7701 initialized\n");
}




// changes background color
void lcd_fill(uint16_t color) {
    if (panel_handle == NULL) {
        printf("ERROR: panel_handle is NULL\n");
        return;
    }

    size_t pixel_count = LCD_H_RES * LCD_V_RES;

    uint16_t* buffer = heap_caps_malloc(
        pixel_count * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM
    );

    if (buffer == NULL) {
        printf("ERROR: Could not allocate screen buffer\n");
        return;
    }

    for (size_t i = 0; i < pixel_count; i++) {
        buffer[i] = color;
    }

    ESP_ERROR_CHECK(
        esp_lcd_panel_draw_bitmap(
            panel_handle,
            0,
            0,
            LCD_H_RES,
            LCD_V_RES,
            buffer
        )
    );

    free(buffer);
}


// enables backlight
void lcd_backlight_init(uint16_t duty) {
    ledc_timer_config_t timer_conf =
    {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT, //256
      .timer_num = LEDC_TIMER_3,
      .freq_hz = 50 * 1000,
      .clk_cfg = LEDC_SLOW_CLK_RC_FAST,
    };
    ledc_channel_config_t ledc_conf =
    {
      .gpio_num = LCD_BL,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_1,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_3,
      .duty = duty,
      .hpoint = 0,
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_timer_config(&timer_conf));
    ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_channel_config(&ledc_conf));
}


// call function
void lcd_init(void) {

    printf("Initializing LCD...\n");

    lcd_backlight_init(LCD_PWM_MODE_255);

    rgb_panel_init();

    printf("LCD initialized\n");
}

// LVGL specifics

static void lvgl_tick(void* arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_flush_cb(
    lv_display_t* disp,
    const lv_area_t* area,
    uint8_t* color_p) {
    esp_lcd_panel_handle_t panel =
        (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);

    esp_lcd_panel_draw_bitmap(
        panel,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        color_p
    );
}

static void lvgl_flush_wait_cb(lv_display_t* disp) {
    xSemaphoreTake(flush_done_semaphore, portMAX_DELAY);
}



static void lvgl_task(void* arg) {
    uint32_t delay_ms = LVGL_TASK_MAX_DELAY_MS;

    while (1) {
        if (xSemaphoreTake(lvgl_mux, portMAX_DELAY)) {
            delay_ms = lv_timer_handler();

            xSemaphoreGive(lvgl_mux);
        }

        if (delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            delay_ms = LVGL_TASK_MAX_DELAY_MS;
        }
        else if (delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}


void lcd_lvgl_init(void) {
    printf("Initializing LVGL...\n");

    lv_init();

    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);

    flush_done_semaphore = xSemaphoreCreateBinary();
    assert(flush_done_semaphore);


    lv_display_t* disp =
        lv_display_create(LCD_H_RES, LCD_V_RES);

    assert(disp);


    lv_display_set_user_data(disp, panel_handle);


    lv_display_set_flush_cb(disp, lvgl_flush_cb);


    lv_display_set_flush_wait_cb(disp, lvgl_flush_wait_cb);


    size_t buffer_size =
        LCD_H_RES * LCD_V_RES * sizeof(lv_color_t);

    void* buf_1 = heap_caps_malloc(
        buffer_size,
        MALLOC_CAP_SPIRAM
    );

    printf("buf_1 = %p\n", buf_1);

    void* buf_2 = heap_caps_malloc(
        buffer_size,
        MALLOC_CAP_SPIRAM
    );

    printf("buf_2 = %p\n", buf_2);

    assert(buf_1);
    assert(buf_2);

    lv_display_set_buffers(
        disp,
        buf_1,
        buf_2,
        buffer_size,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(
        esp_timer_create(
            &tick_args,
            &lvgl_tick_timer
        )
    );


    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            lvgl_tick_timer,
            LVGL_TICK_PERIOD_MS * 1000
        )
    );

    BaseType_t result = xTaskCreate(
        lvgl_task,
        "LVGL",
        LVGL_TASK_STACK_SIZE,
        NULL,
        LVGL_TASK_PRIORITY,
        NULL
    );

    assert(result == pdPASS);

    printf("LVGL initialized\n");
}