#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TWAI_TX_GPIO       21
#define TWAI_RX_GPIO       20
#define TWAI_BITRATE       500000

#define OBD_REQUEST_ID     0x7DF
#define OBD_RESPONSE_ID    0x7E8

static const char *TAG = "OBD_RPM";


typedef struct {
    twai_frame_t frame;
    uint8_t data[TWAI_FRAME_MAX_LEN];
} rx_data_t;


typedef struct {
    twai_node_handle_t node_hdl;

    rx_data_t rx;

    SemaphoreHandle_t rx_semaphore;

} obd_context_t;


/*
 * RX callback
 */
static bool IRAM_ATTR twai_rx_callback(
    twai_node_handle_t handle,
    const twai_rx_done_event_data_t *edata,
    void *user_ctx)
{
    obd_context_t *ctx = (obd_context_t *)user_ctx;

    BaseType_t woken = pdFALSE;

    if (twai_node_receive_from_isr(
            handle,
            &ctx->rx.frame) == ESP_OK)
    {
        xSemaphoreGiveFromISR(
            ctx->rx_semaphore,
            &woken
        );
    }

    return (woken == pdTRUE);
}


/*
 * CAN error callback
 */
static bool IRAM_ATTR twai_error_callback(
    twai_node_handle_t handle,
    const twai_error_event_data_t *edata,
    void *user_ctx)
{
    ESP_EARLY_LOGW(
        TAG,
        "CAN error flags: 0x%lx",
        (unsigned long)edata->err_flags.val
    );

    return false;
}


void app_main(void)
{
    printf("\n");
    printf("==============================\n");
    printf("G35 OBD-II RPM Reader\n");
    printf("==============================\n");


    obd_context_t ctx = {0};


    /*
     * Create semaphore
     */
    ctx.rx_semaphore = xSemaphoreCreateBinary();

    assert(ctx.rx_semaphore != NULL);


    /*
     * RX buffer
     */
    ctx.rx.frame.buffer = ctx.rx.data;
    ctx.rx.frame.buffer_len = sizeof(ctx.rx.data);


    /*
     * Configure CAN
     */
    twai_onchip_node_config_t node_config = {

        .io_cfg = {
            .tx = TWAI_TX_GPIO,
            .rx = TWAI_RX_GPIO,
            .quanta_clk_out = GPIO_NUM_NC,
            .bus_off_indicator = GPIO_NUM_NC,
        },

        .bit_timing = {
            .bitrate = TWAI_BITRATE,
        },

        .tx_queue_depth = 5,
    };


    /*
     * Create node
     */
    ESP_ERROR_CHECK(
        twai_new_node_onchip(
            &node_config,
            &ctx.node_hdl
        )
    );


    /*
     * Register callbacks
     */
    twai_event_callbacks_t callbacks = {
        .on_rx_done = twai_rx_callback,
        .on_error = twai_error_callback,
    };


    ESP_ERROR_CHECK(
        twai_node_register_event_callbacks(
            ctx.node_hdl,
            &callbacks,
            &ctx
        )
    );


    /*
     * Enable CAN
     */
    ESP_ERROR_CHECK(
        twai_node_enable(ctx.node_hdl)
    );


    ESP_LOGI(
        TAG,
        "CAN started at %d bps",
        TWAI_BITRATE
    );


    /*
     * OBD-II Mode 01 PID 0C
     *
     * 02 = number of OBD data bytes
     * 01 = Mode 01
     * 0C = Engine RPM
     */
    uint8_t rpm_request[8] = {
        0x02,
        0x01,
        0x0C,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };


    while (1)
    {

        /*
         * Build request
         */
        twai_frame_t tx_frame = {
            .header.id = OBD_REQUEST_ID,
            .buffer = rpm_request,
            .buffer_len = 8,
        };


        /*
         * Send OBD request
         */
        esp_err_t err = twai_node_transmit(
            ctx.node_hdl,
            &tx_frame,
            100
        );


        if (err != ESP_OK)
        {
            ESP_LOGW(
                TAG,
                "Transmit failed: %s",
                esp_err_to_name(err)
            );
        }
        else
        {
            ESP_LOGI(
                TAG,
                "RPM request sent"
            );
        }


        /*
         * Wait for incoming CAN frames.
         *
         * We may receive several normal vehicle
         * CAN messages before the ECU response.
         */
        TickType_t timeout =
            pdMS_TO_TICKS(200);

        TickType_t start = xTaskGetTickCount();


        while (
            (xTaskGetTickCount() - start) < timeout
        )
        {

            TickType_t elapsed =
                xTaskGetTickCount() - start;

            TickType_t remaining =
                timeout - elapsed;


            if (xSemaphoreTake(
                    ctx.rx_semaphore,
                    remaining
                ) != pdTRUE)
            {
                break;
            }


            twai_frame_t *frame =
                &ctx.rx.frame;


            /*
             * Print received frame
             */
            printf(
                "RX ID: 0x%03lX  DLC: %lu  DATA:",
                (unsigned long)frame->header.id,
                (unsigned long)frame->buffer_len
            );


            for (
                uint32_t i = 0;
                i < frame->buffer_len;
                i++
            )
            {
                printf(
                    " %02X",
                    frame->buffer[i]
                );
            }


            printf("\n");


            /*
             * Look specifically for ECU response.
             */
            if (
                frame->header.id == OBD_RESPONSE_ID &&
                frame->buffer_len >= 5 &&
                frame->buffer[1] == 0x41 &&
                frame->buffer[2] == 0x0C
            )
            {

                uint16_t raw_rpm =
                    ((uint16_t)frame->buffer[3] << 8) |
                    frame->buffer[4];


                uint16_t rpm =
                    raw_rpm / 4;


                printf(
                    "\n==============================\n"
                );

                printf(
                    "ENGINE RPM: %u\n",
                    rpm
                );

                printf(
                    "==============================\n\n"
                );

                break;
            }
        }


        /*
         * Wait before asking again.
         */
        vTaskDelay(
            pdMS_TO_TICKS(500)
        );
    }
}