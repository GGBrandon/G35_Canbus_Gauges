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


// testing vars
static bool testingMode = true;
static lv_obj_t* engine_load_title;
static lv_obj_t* engine_load_label;

static lv_obj_t* coolant_temp_title;
static lv_obj_t* coolant_temp_label;





// testing for coolant and engine load
void sensorTestUI(void) {

    printf("unlocked");

    lv_obj_t* screen = lv_screen_active();

    // Black background object
    lv_obj_t* obj = lv_obj_create(screen);
    lv_obj_set_size(obj, 280, 400);
    lv_obj_set_style_bg_color(obj, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_center(obj);

    printf("black rect");

    // Engine load title
    engine_load_title = lv_label_create(screen);
    lv_label_set_text(engine_load_title, "ENGINE LOAD");
    lv_obj_set_style_text_font(engine_load_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(engine_load_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(engine_load_title, LV_ALIGN_CENTER, 0, -120);

    // Engine load value
    engine_load_label = lv_label_create(screen);
    lv_label_set_text(engine_load_label, "-- %");
    lv_obj_set_style_text_color(engine_load_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(engine_load_label, &lv_font_montserrat_32, 0);
    lv_obj_align(engine_load_label, LV_ALIGN_CENTER, 0, -70);

    // Coolant temperature title
    coolant_temp_title = lv_label_create(screen);
    lv_label_set_text(coolant_temp_title, "COOLANT TEMP");
    lv_obj_set_style_text_font(coolant_temp_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(coolant_temp_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(coolant_temp_title, LV_ALIGN_CENTER, 0, 20);

    // Coolant temperature value
    coolant_temp_label = lv_label_create(screen);
    lv_label_set_text(coolant_temp_label, "-- F");
    lv_obj_set_style_text_color(coolant_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(coolant_temp_label, &lv_font_montserrat_32, 0);
    lv_obj_align(coolant_temp_label, LV_ALIGN_CENTER, 0, 70);
}



// custom gauge ui setup
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

    update_needle(value); // move needle

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(objects.oilLabel, buf);


}


// runs updates for sensor testing
void sensorTestLoop(void) {

    float value = 0.0f;
    char text[32];

    // Engine load
    esp_err_t err =
        can_sender_request_sensor(SENSOR_ENGINE_LOAD);

    if (err == ESP_OK) {
        err = can_sender_get_sensor(
            SENSOR_ENGINE_LOAD,
            &value);
    }

    if (err == ESP_OK) {
        snprintf(text, sizeof(text), "%.1f %%", value);
        lv_label_set_text(engine_load_label, text);
    }
    else {
        lv_label_set_text(engine_load_label, "-- %");
    }

    // Coolant temperature
    err =
        can_sender_request_sensor(SENSOR_COOLANT_TEMP);

    if (err == ESP_OK) {
        err = can_sender_get_sensor(
            SENSOR_COOLANT_TEMP,
            &value);
    }

    if (err == ESP_OK) {
        snprintf(text, sizeof(text), "%.1f F", value);
        lv_label_set_text(coolant_temp_label, text);
    }
    else {
        lv_label_set_text(coolant_temp_label, "-- F");
    }
}





// runs updates for custom oil gauge 
void oilTempLoop() {

    float oil_temp;
    char oil_temp_text[16];


    if (can_sender_request_sensor(SENSOR_OIL_TEMP) == ESP_OK) {

        if (can_sender_get_sensor(SENSOR_OIL_TEMP, &oil_temp) == ESP_OK) {
            if (lvgl_lock(-1)) {

                update_gauge(oil_temp);
                lvgl_unlock();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(60));
    }


}

void app_main(void) {

    // Error check
    ESP_ERROR_CHECK(can_sender_init());

    lcd_init();
    lcd_lvgl_init();

    // which ui to setup
    if (lvgl_lock(-1)) {
        if (testingMode) {
            sensorTestUI();

        }
        else {
            ui_init();
            
        }
        lvgl_unlock();

    }

    //which update to loop through
    while (1) {

        if (lvgl_lock(-1)) {

            if (testingMode) {

                sensorTestLoop();
            }
            else {

                oilTempLoop();
            }
            lvgl_unlock();
        }
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

