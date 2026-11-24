#ifndef FLOATSAT_CORE_PRIV_H
#define FLOATSAT_CORE_PRIV_H

#include <string.h>

#include "fs_commands.h"
#include "floatsat_core_types.h"



// floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);
floatsat_err_t Core_SetNextMission(floatsat_core_t *handle, uint8_t mission);
floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd);

floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info);
floatsat_err_t Core_EditPhoto(floatsat_core_t *handle, photo_info_t *photo, uint8_t index, uint32_t flags);
floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle);
floatsat_err_t Core_RemovePhoto(floatsat_core_t *handle, uint8_t index);

floatsat_err_t Core_ReadIMU(floatsat_core_t *handle, uint32_t flags);
floatsat_err_t Core_IMUCalibrate(floatsat_core_t *handle, uint32_t flags);
floatsat_err_t Core_IMUCalibrate_Gyro(floatsat_core_t *handle);

floatsat_err_t Core_Sysctl(floatsat_core_t *handle, floatsat_cmd_t *cmd);

void Core_PrintPhotoInfo(floatsat_core_t *handle);

#endif