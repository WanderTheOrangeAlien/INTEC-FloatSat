#ifndef DATA_BUS_H
#define DATA_BUS_H

#include "floatsat_error.h"
#include "floatsat_types.h"

typedef struct data_bus_t data_bus_t;

typedef struct data_bus_writer_t data_bus_writer_t;

typedef struct data_bus_reader_t data_bus_reader_t;

floatsat_err_t DataBus_GetWriter_IMU(data_bus_writer_t **writer);
floatsat_err_t DataBus_WriteIMU(data_bus_writer_t *writer, const IMU_data_t *imu_data);

floatsat_err_t DataBus_InitReader(data_bus_reader_t *out);

floatsat_err_t DataBus_ReadIMU(data_bus_reader_t *reader, IMU_data_t *imu_data, timestamp_t *timestamp);

#endif