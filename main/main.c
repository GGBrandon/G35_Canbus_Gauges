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

#include "ui.h"
#include "screens.h"

lv_obj_t* oil_label;


void update_gauge(int value) {
    lv_arc_set_value(objects.obj0, value);

    lv_color_t color;

    if (value < 180) {
        color = lv_color_hex(0x4CD0FC);  // Cyan ish
    }
    else if (value <= 230) {
        color = lv_color_hex(0x00FF3B);  // Green
    }
    else {
        color = lv_color_hex(0xFF0000);  // Red
    }

    lv_obj_set_style_arc_color(
        objects.obj0,
        color,
        LV_PART_INDICATOR | LV_STATE_DEFAULT
    );

    update_needle(value); //move needle

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(objects.oilLabel, buf);


}



void app_main(void) {

    float oil_temp;

    lcd_init();
    lcd_lvgl_init();


    if (lvgl_lock(-1)) {

        /*
        printf("unlocked");
        lv_obj_t* screen = lv_screen_active();

        // Black background object
        lv_obj_t* obj = lv_obj_create(screen);
        lv_obj_set_size(obj, 280, 400);
        lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_BLACK), 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_center(obj);
        printf("black rect");

        // Oil Temp title
        lv_obj_t* oil_title = lv_label_create(screen);
        lv_label_set_text(oil_title, "OIL TEMP");
        lv_obj_set_style_text_font(oil_title, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(oil_title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(oil_title, LV_ALIGN_CENTER, 0, -100);

        // Oil temperature value
        oil_label = lv_label_create(screen);
        lv_label_set_text(oil_label, "-- F");
        lv_obj_set_style_text_color(oil_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(oil_label, LV_ALIGN_CENTER, 0, -30);

        */

        ui_init();
        lvgl_unlock();
    }




    // Error check
    ESP_ERROR_CHECK(can_sender_init());

    //ui_init();

    // gauge example
    float i = 60;
    char oil_temp_text[16];


    while (i < 250) {

    if (lvgl_lock(-1)) {
        update_gauge(i);

        snprintf(
            oil_temp_text,
            sizeof(oil_temp_text),
            "%.0f F",
            (double)i
        );

        // lv_label_set_text(oil_label, oil_temp_text);

        lvgl_unlock();
    }

    printf("updoot\n");

    i += 1;
    vTaskDelay(pdMS_TO_TICKS(60));
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