#pragma once
#include <string.h>
#include "vl53l0x_def.h"
#include "driver/i2c_master.h"
#define _LOG_FUNCTION_START(...) ((void)0)
#define _LOG_FUNCTION_END(...) ((void)0)
#define _LOG_FUNCTION_END_FMT(...) ((void)0)
#define VL53L0X_COPYSTRING(dst, src) strcpy(dst, src)
typedef struct { VL53L0X_DevData_t Data; uint8_t I2cDevAddr; i2c_master_dev_handle_t handle; } VL53L0X_Dev_t;
typedef VL53L0X_Dev_t *VL53L0X_DEV;
#define PALDevDataGet(dev, field) ((dev)->Data.field)
#define PALDevDataSet(dev, field, value) ((dev)->Data.field = (value))
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV, uint8_t, uint8_t *, uint32_t);
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV, uint8_t, uint8_t *, uint32_t);
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV, uint8_t, uint8_t);
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV, uint8_t, uint16_t);
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV, uint8_t, uint32_t);
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV, uint8_t, uint8_t *);
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV, uint8_t, uint16_t *);
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV, uint8_t, uint32_t *);
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV, uint8_t, uint8_t, uint8_t);
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV);
