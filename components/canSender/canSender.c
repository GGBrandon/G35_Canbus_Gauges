#include "canSender.h"

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#define TWAI_TX_GPIO 21
#define TWAI_RX_GPIO 20
#define TWAI_BITRATE 500000

#define OBD_FUNCTION_ID 0x7DF // Standard request
#define OBD_ENGINE_ID 0x7E0   // Nissan ECU specific request
#define OBD_RESPONSE_ID 0x7E8 // ECU response

static const char* TAG = "CAN_SENDER";

typedef enum {
    PID_TYPE_OBD01,   // OBD-II Service 01
    PID_TYPE_NISSAN21 // Nissan Service 21
} pid_type_t;

typedef struct {
    pid_id_t id;
    const char* name;
    pid_type_t type;
    uint16_t pid;
} pid_definition_t;

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
static const pid_definition_t pid_definitions[PID_COUNT] = {
    { .id = PID_RPM,
     .name = "RPM",
     .type = PID_TYPE_OBD01,
     .pid = 0x0C },

    { .id = PID_OIL_TEMP,
     .name = "Oil Temp",
     .type = PID_TYPE_NISSAN21,
     .pid = 0xE004 } };

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


esp_err_t can_sender_request_pid(pid_id_t pid_id) {

    if (pid_id >= PID_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    const pid_definition_t* pid =
        &pid_definitions[pid_id];

    if (pid->type == PID_TYPE_OBD01) {

        return can_sender_request_OBD01(
            (uint8_t)pid->pid);
    }

  if (pid->type == PID_TYPE_NISSAN21) {

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
        pid->pid);
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

static esp_err_t can_sender_get_pid_raw(
    pid_id_t pid_id,
    uint8_t* data,
    uint8_t* data_len) {

    if (data == NULL ||
        data_len == NULL ||
        pid_id >= PID_COUNT) {

        return ESP_ERR_INVALID_ARG;
    }

    const pid_definition_t* pid =
        &pid_definitions[pid_id];

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

        printf(
            "CAN ID: 0x%03lX DATA:",
            frame->header.id);

        for (int i = 0;
            i < frame->buffer_len;
            i++) {

            printf(
                " %02X",
                frame->buffer[i]);
        }

        printf("\n");

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
            pid->type == PID_TYPE_OBD01 &&

            frame->header.id == OBD_RESPONSE_ID &&

            frame->buffer_len >= 3 &&

            frame->buffer[1] == 0x41 &&

            frame->buffer[2] ==
            (uint8_t)pid->pid
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
            pid->type == PID_TYPE_NISSAN21 &&

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

esp_err_t can_sender_get_pid(
    pid_id_t pid_id,
    float* value) {

    if (value == NULL ||
        pid_id >= PID_COUNT) {

        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[8];

    uint8_t data_len = 0;

    esp_err_t err =
        can_sender_get_pid_raw(
            pid_id,
            data,
            &data_len);

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

    if (pid_id == PID_RPM) {

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

    if (pid_id == PID_OIL_TEMP) {

        if (data_len < 1) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        *value =
            //Ex: (123 - 50) * (9/5) + 32 = 163.4 F
            ((float)data[0] - 50.0f) * 9.0f / 5.0f + 32.0f;

        return ESP_OK;
    }

    return ESP_ERR_INVALID_ARG;
}    