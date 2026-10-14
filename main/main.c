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


void update_gauge(int value)
{
    lv_arc_set_value(objects.obj0, value);

    lv_color_t color;

    if (value < 180) {
        color = lv_color_hex(0x4CD0FC);  // Cyan ish
    } else if (value <= 230) {
        color = lv_color_hex(0x00FF3B);  // Green
    } else {
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



void app_main(void)
{

    float oil_temp;

    lcd_init();
    lcd_lvgl_init();

    ui_init();

    // gauge example
    int i = 60;
    while(i < 250) {

        update_gauge(i);
        lv_timer_handler();
        i+= 10;
        vTaskDelay(pdMS_TO_TICKS(3000));

       
       
       

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