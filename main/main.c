#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "lvgl.h"

//components
#include "canSender.h"
#include "lcdDisplay.h"
#include "lcdConfig.h"




void app_main(void)
{

    float oil_temp;

    lcd_init();
    lcd_lvgl_init();

    lv_obj_t *screen = lv_screen_active();

    // Black background object
    lv_obj_t *obj = lv_obj_create(screen);
    lv_obj_set_size(obj, 280, 400);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_center(obj);

    // Oil Temp title
    lv_obj_t *oil_title = lv_label_create(screen);
    lv_label_set_text(oil_title, "OIL TEMP");
    lv_obj_set_style_text_font(oil_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(oil_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(oil_title, LV_ALIGN_CENTER, 0, -100);

    // Oil temperature value
    lv_obj_t *oil_label = lv_label_create(screen);
    lv_label_set_text(oil_label, "-- F");
    lv_obj_set_style_text_color(oil_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(oil_label, LV_ALIGN_CENTER, 0, 20);

    // Error check
    ESP_ERROR_CHECK(can_sender_init());


    while (1)
    {
        

        // Request oil temperature over CAN
        if (can_sender_request_pid(PID_OIL_TEMP) == ESP_OK)
        {
            if (can_sender_get_pid(PID_OIL_TEMP, &oil_temp) == ESP_OK)
            {
                char oil_temp_text[16];

                snprintf(
                    oil_temp_text,
                    sizeof(oil_temp_text),
                    "%.0f F",
                    oil_temp
                );

                
                // Update LVGL label
                lv_label_set_text(oil_label, oil_temp_text);

                printf("OIL TEMP: %.0f F\n", oil_temp);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*

while (1) {

    lcd_fill(COLOR_GREEN);

    vTaskDelay(pdMS_TO_TICKS(1000));

    lcd_fill(COLOR_BLUE);


    vTaskDelay(pdMS_TO_TICKS(1000));
}

}
*/

/*
    gpio_config_t button_config = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&button_config);

    // Error check
    ESP_ERROR_CHECK(can_sender_init());

    // init display
    lcd_init();
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Ready"); // print to screen



    int current_display = 2;


    while (1) {

        if (gpio_get_level(BUTTON_GPIO) == 0) {
            lcd_clear();
            current_display++;
            printf("press");

            if (current_display > 3) {
                current_display = 1;

            }

            // wait for button release
            while (gpio_get_level(BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }



        if (current_display == 1) {
            float rpm;


            lcd_set_cursor(0, 0);
            lcd_print("ENGINE RPM");

            if (can_sender_request_pid(PID_RPM) == ESP_OK) {
                if (can_sender_get_pid(PID_RPM, &rpm) == ESP_OK) {
                    char rpm_text[16];

                    snprintf(
                        rpm_text,
                        sizeof(rpm_text),
                        "%.0f",
                        rpm
                    );

                    lcd_set_cursor(0, 1);
                    lcd_print(rpm_text);

                    //print to console
                    printf(
                        "ENGINE RPM: %.0f\n",
                        rpm
                    );
                }
            }
        }



        else if (current_display == 2) {
            float oil_temp;

            lcd_set_cursor(0, 0);
            lcd_print("OIL TEMP");

            if (can_sender_request_pid(PID_OIL_TEMP) == ESP_OK) {
                if (can_sender_get_pid(PID_OIL_TEMP, &oil_temp) == ESP_OK) {
                    char oil_temp_text[16];

                    snprintf(
                        oil_temp_text,
                        sizeof(oil_temp_text),
                        "%.0f F",
                        oil_temp
                    );




                    lcd_set_cursor(0, 1);
                    lcd_print(oil_temp_text);

                    //print to console
                    printf(
                        "OIL TEMP: %.0f F\n",
                        oil_temp
                    );
                }
            }

        }
        // check to see if button works
        else if (current_display == 3) {
            printf("display 3");
            lcd_print("display 3");
        }



        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
*/