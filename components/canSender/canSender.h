#ifndef CAN_SENDER_H
#define CAN_SENDER_H

#include <stdint.h>
#include "esp_err.h"

/**
 * Initialize the CAN/TWAI interface.
 */
esp_err_t can_sender_init(void);

/**
 * Send an OBD-II request for engine RPM.
 */
esp_err_t can_sender_request_rpm(void);

/**
 * Wait for the ECU's RPM response and return RPM.
 *
 * Returns:
 *   ESP_OK       - RPM successfully received
 *   ESP_ERR_TIMEOUT - No valid RPM response received
 */
esp_err_t can_sender_get_rpm(uint16_t *rpm);

#endif // CAN_SENDER_H