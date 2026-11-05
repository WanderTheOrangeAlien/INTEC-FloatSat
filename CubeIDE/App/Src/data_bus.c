#include "data_bus.h"
#include "data_bus_priv.h"

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define DATABUS_WRITER_TYPE_IMU     0x00



/* ======================= Globals ======================== */

static data_bus_t data_bus = {0};
static data_bus_writer_t imu_writer = {
    .acquired       = false,
    .data_bus       = &data_bus,
    .writer_type    = DATABUS_WRITER_TYPE_IMU,
};


/* =============================== Functions ================================ */

floatsat_err_t DataBus_GetWriter_IMU(data_bus_writer_t **writer)
{
    if(writer == NULL){
        return ERR_INVALID_ARG;
    }

    if(imu_writer.acquired){
        return ERR_RESOURCE_BUSY;   
    }

    imu_writer.acquired = true;
    *writer = &imu_writer;

    return ERR_OK;
}

floatsat_err_t DataBus_InitReader(data_bus_reader_t *out)
{
    if(!out){
        return ERR_INVALID_ARG;
    }

    out->data_bus = &data_bus;
    
    return ERR_OK;
}

floatsat_err_t DataBus_WriteIMU(data_bus_writer_t *writer, const IMU_data_t *imu_data)
{
    if(!writer || !imu_data){
        return ERR_INVALID_ARG;
    }

    if(writer->writer_type != DATABUS_WRITER_TYPE_IMU){
        return ERR_DATABUS_INVALID_WRITE;
    }

    taskENTER_CRITICAL();
    writer->data_bus->imu.data = *imu_data;
    writer->data_bus->imu.timestamp_ms = HAL_GetTick();
    taskEXIT_CRITICAL();

    return ERR_OK;
}


floatsat_err_t DataBus_ReadIMU(data_bus_reader_t *reader, IMU_data_t *imu_data, timestamp_t *timestamp)
{
    if(!reader || ! imu_data){
        return ERR_INVALID_ARG;
    }

    taskENTER_CRITICAL();
    *imu_data = reader->data_bus->imu.data;
    *timestamp = reader->data_bus->imu.timestamp_ms;
    taskEXIT_CRITICAL();

    return ERR_OK;
}