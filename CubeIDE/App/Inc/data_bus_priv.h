#ifndef DATA_BUS_PRIV_H
#define DATA_BUS_PRIV_H

#include "floatsat_types.h"

/* ======================= Opaque Struct Definitions ======================== */
typedef struct data_bus_t {
    struct {
        timestamp_t timestamp_ms;
        IMU_data_t data;
    }imu;
    
}data_bus_t;

typedef struct data_bus_writer_t {
    data_bus_t  *data_bus;
    uint8_t     writer_type; // What field of the databus this writer is allowed to modify 
    bool        acquired;
}data_bus_writer_t;

typedef struct data_bus_reader_t {
    data_bus_t *data_bus;
}data_bus_reader_t;

#endif