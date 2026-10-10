#ifndef CAN_SENDER_H
#define CAN_SENDER_H

#include <stdint.h>
#include "esp_err.h"

#define TWAI_TX_GPIO 43
#define TWAI_RX_GPIO 44
#define TWAI_BITRATE 500000

#define OBD_FUNCTION_ID 0x7DF // Standard request
#define OBD_ENGINE_ID 0x7E0   // Nissan ECU specific request
#define OBD_RESPONSE_ID 0x7E8 // ECU response

/**
 * PID types.
 */
typedef enum {
    PID_RPM,
    PID_OIL_TEMP,

    PID_COUNT

} pid_id_t;

/**
 * Initialize the CAN/TWAI interface.
 */
esp_err_t can_sender_init(void);

/**
 * Send a request for a specific PID.
 */
esp_err_t can_sender_request_pid(pid_id_t pid_id);

/**
 * Wait for the ECU response and return the decoded value.
 *
 * Returns:
 *   ESP_OK          - Value successfully received
 *   ESP_ERR_TIMEOUT - No valid response received
 */
esp_err_t can_sender_get_pid(pid_id_t pid_id, float* value);

#endif // CAN_SENDER_H



/**
 * Initialize the CAN/TWAI interface.

esp_err_t can_sender_init(void);


 * Send an OBD-II request for engine RPM.

esp_err_t can_sender_request_rpm(void);


 * Wait for the ECU's RPM response and return RPM.
 *
 * Returns:
 *   ESP_OK       - RPM successfully received
 *   ESP_ERR_TIMEOUT - No valid RPM response received

esp_err_t can_sender_get_rpm(uint16_t *rpm);

#endif // CAN_SENDER_H

 */