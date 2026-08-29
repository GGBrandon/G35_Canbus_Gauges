/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//components
#include "canSender.h"
#include "LCD1602.h"


void app_main(void)
{   
    //Error check
    ESP_ERROR_CHECK(can_sender_init());
    //init display
    lcd_init(); 

    while (1)
    {
        if (can_sender_request_rpm() == ESP_OK)
        {
            uint16_t rpm;

            
            //recieved correct byte
            if (can_sender_get_rpm(&rpm) == ESP_OK)  {

                char rpm_text[16];

               snprintf(rpm_text, sizeof(rpm_text), "%u", rpm);

                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("ENGINE RPM"); //print to screen

                lcd_set_cursor(0,1);
                lcd_print(rpm_text);


                printf("ENGINE RPM: %u\n", rpm); //print to console 
            }
            //did not recieve correct byte
            else {   

                lcd_clear();
                lcd_set_cursor(0,1);
                lcd_print("No RPM response"); //print to screen

                printf("No RPM response\n"); //print to console
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}