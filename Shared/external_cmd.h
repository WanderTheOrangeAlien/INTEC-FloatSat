#ifndef EXTERNAL_CMD_H
#define EXTERNAL_CMD_H

#include <stdint.h>

/*
    Commands for the Companion Microcontroller (ESP32) and the Grond Station
*/

#define EXT_CMD_TAKE_PHOTO      0x00

typedef struct __attribute__((packed)) esp_cmd_t {
    uint8_t cmd_id;
    union {
        uint8_t photo_amount;
    }args;

}esp_cmd_t;

#endif