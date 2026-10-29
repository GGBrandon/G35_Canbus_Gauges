
#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

// sensor ids
typedef enum {
    SENSOR_RPM,
    SENSOR_OIL_TEMP,
    SENSOR_ENGINE_LOAD,
    SENSOR_COOLANT_TEMP,
    SENSOR_COUNT

} sensor_id_t;


// sensor request protocols
typedef enum {
    SENSOR_PROTOCOL_OBD01,
    SENSOR_PROTOCOL_NISSAN21

} sensor_protocol_t;


// sensor response decoding
 

typedef enum {
    SENSOR_DECODE_RPM,
    SENSOR_DECODE_OIL_TEMP,
    SENSOR_DECODE_ENGINE_LOAD,
    SENSOR_DECODE_COOLANT_TEMP,

} sensor_decode_t;


// sensor definitions

typedef struct {
    sensor_id_t id;
    const char* name;
    sensor_protocol_t protocol;
    uint16_t pid;
    sensor_decode_t decode;

} sensor_definition_t;

#endif // SENSOR_CONFIG_H
