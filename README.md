# G35 CAN Bus Gauges

ESP32-C3 based digital gauge project for a 2007 Infiniti G35 VQ35HR.

The project communicates with the vehicle over CAN bus using an SN65HVD230 CAN transceiver and retrieves OBD-II data such as engine RPM. The data is then displayed on an LCD. Currently only displays Engine RPM. Current version has not been tested, but components work individually. 

## Current Features

* ESP32-C3 CAN bus communication
* 500 kbps CAN bitrate
* Standard OBD-II Mode 01 requests
* Engine RPM (PID `0x0C`) retrieval
* 16x2 LCD display
* Modular ESP-IDF component structure
* CAN error reporting

## Hardware

### Microcontroller

* ESP32-C3

### CAN Transceiver

* SN65HVD230

### LCD

* 1602 LCD

## CAN Connections

ESP32-C3:

| ESP32-C3 | SN65HVD230 |
| -------- | ---------- |
| GPIO 21  | TXD        |
| GPIO 20  | RXD        |
| 3.3V     | VCC        |
| GND      | GND        |

The SN65HVD230 CANH/CANL connections are connected to the vehicle CAN bus through the OBD-II connector pinout.

## OBD-II RPM Request

The project uses the standard OBD-II broadcast request ID:

```text
0x7DF
```

The RPM request is:

```text
02 01 0C 00 00 00 00 00
```

Where:

* `02` = number of OBD data bytes
* `01` = Mode 01, current powertrain data
* `0C` = Engine RPM PID

A typical ECU response is:

```text
ID:   0x7E8
DATA: 04 41 0C AA BB 00 00 00
```

The RPM is calculated using:

```text
RPM = ((A × 256) + B) / 4
```

where `A` and `B` are bytes 3 and 4 of the response.

## Project Structure

```text
g35_canbus_gauges/
│
├── CMakeLists.txt
├── sdkconfig
│
├── main/
│   ├── CMakeLists.txt
│   └── main.c
│
└── components/
    │
    ├── canSender/
    │   ├── CMakeLists.txt
    │   ├── canSender.c
    │   └── canSender.h
    │
    └── LCD1602/
        ├── CMakeLists.txt
        ├── LCD1602.c
        └── LCD1602.h
```

The current application:

1. Initializes CAN
2. Initializes the LCD
3. Requests engine RPM
4. Waits for the matching ECU response
5. Decodes RPM
6. Displays RPM
7. Repeats every 500 ms

## Configuration

Current CAN configuration:

```c
#define TWAI_TX_GPIO       21
#define TWAI_RX_GPIO       20
#define TWAI_BITRATE       500000
```

OBD-II IDs:

```c
#define OBD_REQUEST_ID     0x7DF
#define OBD_RESPONSE_ID    0x7E8
```

## Roadmap

Planned functionality includes retrieving additional vehicle data and displaying multiple values on a larger gauge display.

Potential data points:

* Engine oil temperature
* Engine oil pressure
* Coolant temperature
* Engine load
* Intake air temperature
* Fuel-related data
## Development Notes

The vehicle continuously transmits many CAN messages that are unrelated to the requested OBD-II PID.

The CAN receiver therefore does not assume that the next received frame is the requested response. It filters incoming frames and only accepts the expected RPM response:

```text
ID   = 0x7E8
DATA = 04 41 0C ...
```

Other CAN frames are ignored.

CAN errors are still reported through the TWAI error callback.
