#include "canSender.h"

#include <stdint.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TWAI_TX_GPIO       21
#define TWAI_RX_GPIO       20
#define TWAI_BITRATE       500000

#define OBD_REQUEST_ID     0x7DF
#define OBD_RESPONSE_ID    0x7E8

static const char *TAG = "CAN_SENDER";


typedef struct {
    twai_frame_t frame;
    uint8_t data[TWAI_FRAME_MAX_LEN];
} rx_data_t;


typedef struct {
    twai_node_handle_t node_hdl;
    rx_data_t rx;
    SemaphoreHandle_t rx_semaphore;
} can_sender_context_t;


static can_sender_context_t ctx;


/*
 * CAN RX callback
 */
static bool IRAM_ATTR twai_rx_callback(
    twai_node_handle_t handle,
    const twai_rx_done_event_data_t *edata,
    void *user_ctx)
{
    can_sender_context_t *context =
        (can_sender_context_t *)user_ctx;

    BaseType_t woken = pdFALSE;

    if (twai_node_receive_from_isr(
            handle,
            &context->rx.frame) == ESP_OK)
    {
        xSemaphoreGiveFromISR(
            context->rx_semaphore,
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


/*
 * Initialize CAN/TWAI
 */
esp_err_t can_sender_init(void)
{
    ctx.rx_semaphore = xSemaphoreCreateBinary();

    if (ctx.rx_semaphore == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    /*
     * RX frame buffer
     */
    ctx.rx.frame.buffer = ctx.rx.data;
    ctx.rx.frame.buffer_len = sizeof(ctx.rx.data);


    /*
     * Configure TWAI
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
     * Create TWAI node
     */
    esp_err_t err =
        twai_new_node_onchip(
            &node_config,
            &ctx.node_hdl
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /*
     * Register callbacks
     */
    twai_event_callbacks_t callbacks = {
        .on_rx_done = twai_rx_callback,
        .on_error = twai_error_callback,
    };


    err =
        twai_node_register_event_callbacks(
            ctx.node_hdl,
            &callbacks,
            &ctx
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /*
     * Enable TWAI
     */
    err =
        twai_node_enable(ctx.node_hdl);

    if (err != ESP_OK)
    {
        return err;
    }


    ESP_LOGI(
        TAG,
        "CAN started at %d bps",
        TWAI_BITRATE
    );

    return ESP_OK;
}


/*
 * Send OBD-II RPM request
 */
esp_err_t can_sender_request_rpm(void)
{
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


    twai_frame_t tx_frame = {
        .header.id = OBD_REQUEST_ID,
        .buffer = rpm_request,
        .buffer_len = 8,
    };


    return twai_node_transmit(
        ctx.node_hdl,
        &tx_frame,
        100
    );
}


/*
 * Wait for a valid RPM response
 */
esp_err_t can_sender_get_rpm(uint16_t *rpm)
{
    if (rpm == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    /*
     * Wait up to 200 ms for CAN frames.
     *
     * There may be many other CAN messages,
     * so we ignore everything that isn't
     */
    TickType_t timeout =
        pdMS_TO_TICKS(200);

    TickType_t start =
        xTaskGetTickCount();


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
         * Only accept:
         *
         * ID: 0x7E8
         * 04 41 0C AA BB
         */
        if (
            frame->header.id == OBD_RESPONSE_ID &&
            frame->buffer_len >= 5 &&
            frame->buffer[0] == 0x04 &&
            frame->buffer[1] == 0x41 &&
            frame->buffer[2] == 0x0C
        )
        {
            uint16_t raw_rpm =
                ((uint16_t)frame->buffer[3] << 8) |
                frame->buffer[4];


            *rpm = raw_rpm / 4;

            return ESP_OK;
        }
    }


    return ESP_ERR_TIMEOUT;
}