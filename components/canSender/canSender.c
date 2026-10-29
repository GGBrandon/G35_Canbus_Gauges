#include "canSender.h"

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

/*
 * Sensor definitions
 *
 * RPM:
 * Service = 01
 * PID     = 0C
 *
 * Oil Temp:
 * Service = 21
 * Nissan-specific PID = E0 04 01
 */


static const sensor_definition_t sensor_definitions[SENSOR_COUNT] = {
    {
        .id = SENSOR_RPM,
        .name = "RPM",
        .protocol = SENSOR_PROTOCOL_OBD01,
        .pid = 0x0C,
        .decode = SENSOR_DECODE_RPM
    },
    {
        .id = SENSOR_OIL_TEMP,
        .name = "Oil Temp",
        .protocol = SENSOR_PROTOCOL_NISSAN21,
        .pid = 0xE004,
        .decode = SENSOR_DECODE_OIL_TEMP
    },
    {
        .id = SENSOR_ENGINE_LOAD,
        .name = "Engine Load",
        .protocol = SENSOR_PROTOCOL_OBD01,
        .pid = 0x04,
        .decode = SENSOR_DECODE_ENGINE_LOAD
    },
    {
        .id = SENSOR_COOLANT_TEMP,
        .name = "Coolant Temp",
        .protocol = SENSOR_PROTOCOL_OBD01,
        .pid = 0x05,
        .decode = SENSOR_DECODE_COOLANT_TEMP
    }
};


static const char* TAG = "CAN_SENDER";

/*
 * CAN RX DATA
 */

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
    const twai_rx_done_event_data_t* edata,
    void* user_ctx) {

    can_sender_context_t* context =
        (can_sender_context_t*)user_ctx;

    BaseType_t woken = pdFALSE;

    if (twai_node_receive_from_isr(
        handle,
        &context->rx.frame) == ESP_OK) {

        xSemaphoreGiveFromISR(
            context->rx_semaphore,
            &woken);
    }

    return (woken == pdTRUE);
}

/*
 * CAN error callback
 *
 * Mainly used to check if CAN transceiver is working correctly
 */
static bool IRAM_ATTR twai_error_callback(
    twai_node_handle_t handle,
    const twai_error_event_data_t* edata,
    void* user_ctx) {

    ESP_EARLY_LOGW(
        TAG,
        "CAN error flags: 0x%lx",
        (unsigned long)edata->err_flags.val);

    return false;
}

/*
 * Initialize CAN/TWAI
 */
esp_err_t can_sender_init(void) {

    ctx.rx_semaphore = xSemaphoreCreateBinary();
    
    if (ctx.rx_semaphore == NULL) {
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
            &ctx.node_hdl);

    if (err != ESP_OK) {
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
            &ctx);

    if (err != ESP_OK) {
        return err;
    }

    /*
     * Enable TWAI
     */
    err =
        twai_node_enable(ctx.node_hdl);

    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(
        TAG,
        "CAN started at %d bps",
        TWAI_BITRATE);

    return ESP_OK;
}

/*
 * Send OBD-II REQUESTS STANDARD
 */
esp_err_t can_sender_request_OBD01(uint8_t pid) {

    uint8_t request[8] = {
        0x02,
        0x01,
        pid,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };

    twai_frame_t tx_frame = {
        .header.id = OBD_FUNCTION_ID,
        .buffer = request,
        .buffer_len = 8,
    };

    return twai_node_transmit(
        ctx.node_hdl,
        &tx_frame,
        100);
}
/*
 * Start Nissan diagnostic session
 *
 * Request:
 *
 * 7E0 -> 02 10 C0 00 00 00 00 00
 *
 * Response:
 *
 * 7E8 -> 02 50 C0 00 00 00 00 00
 */
static esp_err_t can_sender_start_diagnostic_session(void)
{
    uint8_t request[8] = {
        0x02,
        0x10,
        0xC0,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };

    twai_frame_t tx_frame = {
        .header.id = OBD_ENGINE_ID,
        .buffer = request,
        .buffer_len = 8,
    };

    printf(
        "TX 0x7E0: 02 10 C0 00 00 00 00 00\n"
    );

    return twai_node_transmit(
        ctx.node_hdl,
        &tx_frame,
        100);
}
/*
 * Wait for Nissan diagnostic session response
 *
 * Expected:
 *
 * 7E8 -> 02 50 C0 00 00 00 00 00
 */
static esp_err_t can_sender_wait_for_diagnostic_session(void)
{
    TickType_t timeout = pdMS_TO_TICKS(200);

    TickType_t start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < timeout) {

        TickType_t elapsed =
            xTaskGetTickCount() - start;

        TickType_t remaining =
            timeout - elapsed;

        if (xSemaphoreTake(
            ctx.rx_semaphore,
            remaining) != pdTRUE) {

            break;
        }

        twai_frame_t* frame =
            &ctx.rx.frame;

        /*
         * We only care about the ECU response
         */
        if (frame->header.id != OBD_RESPONSE_ID) {
            continue;
        }

        /*
         * Check for:
         *
         * 02 50 C0
         */
        if (frame->buffer_len >= 3 &&
            frame->buffer[1] == 0x50 &&
            frame->buffer[2] == 0xC0) {

            //for console debugging purposes
            printf(
                "7E8: 02 50 C0 00 00 00 00 00\n"
            );

            //same
            printf(
                "Diagnostic session started.\n"
            );

            return ESP_OK;
        }
    }
    //same
    printf(
        "Diagnostic session response timeout.\n"
    );

    return ESP_ERR_TIMEOUT;
}

/*
 * Send NISSAN SERVICE 21
 *
 * Oil Temperature:
 *
 * Request:
 *
 * 7E0 -> 04 21 E0 04 01 00 00 00
 *
 * Response:
 *
 * 7E8 -> 03 61 00 XX 00 00 00 00
 *
 * XX = raw oil temperature value
 */

static esp_err_t can_sender_request_nissan21(uint16_t pid) {

    uint8_t request[8] = {

        0x04,

        0x21,

        (uint8_t)(pid >> 8),

        (uint8_t)(pid & 0xFF),

        0x01,

        0x00,

        0x00,

        0x00
    };

    twai_frame_t tx_frame = {

        .header.id = OBD_ENGINE_ID,

        .buffer = request,

        .buffer_len = 8,
    };

    return twai_node_transmit(
        ctx.node_hdl,
        &tx_frame,
        100);
}


esp_err_t can_sender_request_sensor(sensor_id_t sensor) {

    if (sensor >= SENSOR_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    const sensor_definition_t* definition =
        &sensor_definitions[sensor];


      if (definition->protocol == SENSOR_PROTOCOL_OBD01) {

        return can_sender_request_OBD01(
            (uint8_t)definition->pid);
    }

  if (definition->protocol == SENSOR_PROTOCOL_NISSAN21) {

    /*
     * Start Nissan diagnostic session first
     */
    esp_err_t err =
        can_sender_start_diagnostic_session();

    if (err != ESP_OK) {
        return err;
    }

    /*
     * Wait for:
     *
     * 7E8 -> 02 50 C0
     */
    err =
        can_sender_wait_for_diagnostic_session();

    if (err != ESP_OK) {
        return err;
    }

    /*
     * Now request the Nissan PID
     */
    return can_sender_request_nissan21(
        definition->pid);
}

    return ESP_ERR_INVALID_ARG;
}

/*
 * This handles both:
 *
 * Standard OBD:
 *
 * Nissan Service 21:
 *
 */

static esp_err_t can_sender_get_sensor_raw(
    sensor_id_t sensor,
    uint8_t* data,
    uint8_t* data_len) {

    if (data == NULL ||
        data_len == NULL ||
        sensor >= SENSOR_COUNT) {

        return ESP_ERR_INVALID_ARG;
    }

    const sensor_definition_t* definition =
        &sensor_definitions[sensor];

    TickType_t timeout =
        pdMS_TO_TICKS(200);

    TickType_t start =
        xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < timeout) {

        TickType_t elapsed =
            xTaskGetTickCount() - start;

        TickType_t remaining =
            timeout - elapsed;

        if (xSemaphoreTake(
            ctx.rx_semaphore,
            remaining) != pdTRUE) {

            break;
        }

        twai_frame_t* frame =
            &ctx.rx.frame;

      //  printf(
        //    "CAN ID: 0x%03lX DATA:",
          //  frame->header.id);

        for (int i = 0;
            i < frame->buffer_len;
            i++) {

            //printf(
              //  " %02X",
                //frame->buffer[i]);
        }

       // printf("\n");

        /*
         * Make sure this came from ECU
         */
        if (frame->header.id == OBD_RESPONSE_ID) {

            printf("ECU RESPONSE: ");

            for (int i = 0;
                i < frame->buffer_len;
                i++) {

                printf(
                    "%02X ",
                    frame->buffer[i]);
            }

            printf("\n");
        }

        /*
         * Standard Request for OBD01
         */

        if (
            definition->protocol == SENSOR_PROTOCOL_OBD01 &&

            frame->header.id == OBD_RESPONSE_ID &&

            frame->buffer_len >= 3 &&

            frame->buffer[1] == 0x41 &&

            frame->buffer[2] ==
            (uint8_t)definition->pid
            ) {

            uint8_t payload_len =
                frame->buffer_len - 3;

            memcpy(
                data,
                &frame->buffer[3],
                payload_len);

            *data_len =
                payload_len;

            return ESP_OK;
        }

        /*
         * NISSAN SERVICE 21
         */

        if (
            definition->protocol == SENSOR_PROTOCOL_NISSAN21 &&

            frame->header.id == OBD_RESPONSE_ID &&

            frame->buffer_len >= 4 &&

            frame->buffer[1] == 0x61 &&

            frame->buffer[2] == 0x00
            ) {

            uint8_t payload_len =
                frame->buffer_len - 3;

            memcpy(
                data,
                &frame->buffer[3],
                payload_len);

            *data_len =
                payload_len;

            return ESP_OK;
        }
    }

    return ESP_ERR_TIMEOUT;
}

/*
 * DECODING
 *
 * Converts raw CAN data into a useful value.
 *
 */

esp_err_t can_sender_get_sensor
(sensor_id_t sensor, float* value) {

    if (value == NULL || sensor >= SENSOR_COUNT) {

        return ESP_ERR_INVALID_ARG;
    }

    const sensor_definition_t* definition =
     &sensor_definitions[sensor];


    uint8_t data[8];

    uint8_t data_len = 0;

    esp_err_t err = can_sender_get_sensor_raw(sensor, data, &data_len);

    if (err != ESP_OK) {
        return err;
    }

    /*
     * ========================================
     * RPM
     * ========================================
     *
     * Response:
     *
     * 41 0C A B
     *
     * RPM = ((A * 256) + B) / 4
     */

    if (definition->decode == SENSOR_DECODE_RPM) {

        if (data_len < 2) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        uint16_t raw_rpm =
            ((uint16_t)data[0] << 8) |
            data[1];

        *value =
            raw_rpm / 4.0f;

        return ESP_OK;
    }

/*
 * Nissan Service 21 - Oil Temp
 *
 * Request:
 * 7E0 -> 04 21 E0 04 01 00 00 00
 *
 * Response:
 * 7E8 -> 03 61 00 XX 00 00 00 00
 *
 * XX = raw temperature value
 */

    if (definition->decode == SENSOR_DECODE_OIL_TEMP) {

        if (data_len < 1) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        *value =
            //Ex: (123 - 50) * (9/5) + 32 = 163.4 F
            ((float)data[0] - 50.0f) * 9.0f / 5.0f + 32.0f;

        return ESP_OK;
    }

    if (definition->decode == SENSOR_DECODE_ENGINE_LOAD) {

        if (data_len < 1) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        *value =
            (float)data[0] * 100.0f / 255.0f;

        return ESP_OK;
    }
    /*
     * Coolant Temperature
     *
     * PID: 05
     * Response: 41 05 A
     *
     * Temperature = A - 40 (Celsius)
     */

    if (definition->decode == SENSOR_DECODE_COOLANT_TEMP) {

        if (data_len < 1) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        float temp_c =
            (float)data[0] - 40.0f;

        *value =
            temp_c * 9.0f / 5.0f + 32.0f;

        return ESP_OK;
    }
    

    return ESP_ERR_INVALID_ARG;
}    