#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "vl53l0x_api.h"
#define INIT_STEP(call) do {int e=(call);printf("INIT %s status=%d\n",#call,e);if(e)return;}while(0)
void app_main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 printf("READY range-test v3 live-single SDA8 SCL9 100kHz; no motor/network code\n");
 i2c_master_bus_config_t cfg={.i2c_port=0,.sda_io_num=8,.scl_io_num=9,.clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
 i2c_master_bus_handle_t bus;ESP_ERROR_CHECK(i2c_new_master_bus(&cfg,&bus));
 int found=0;for(int a=8;a<120;a++){if(i2c_master_probe(bus,a,50)==ESP_OK){printf("I2C_FOUND 0x%02x\n",a);if(a==0x29)found=1;}}
 if(!found){printf("FAIL no device at 0x29\n");return;}
 VL53L0X_Dev_t dev={.I2cDevAddr=0x52};VL53L0X_DEV d=&dev;
 i2c_device_config_t dc={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=0x29,.scl_speed_hz=100000};
 ESP_ERROR_CHECK(i2c_master_bus_add_device(bus,&dc,&dev.handle));
 uint8_t id=0;INIT_STEP(VL53L0X_RdByte(d,0xc0,&id));printf("MODEL_ID 0x%02x\n",id);if(id!=0xee)return;
 /* Reset the sensor too: resetting ESP32 alone leaves its state powered. */
 INIT_STEP(VL53L0X_WrByte(d,0xbf,0));vTaskDelay(pdMS_TO_TICKS(10));
 INIT_STEP(VL53L0X_WrByte(d,0xbf,1));vTaskDelay(pdMS_TO_TICKS(10));
 INIT_STEP(VL53L0X_RdByte(d,0xc0,&id));printf("MODEL_ID_AFTER_RESET 0x%02x\n",id);if(id!=0xee)return;
 INIT_STEP(VL53L0X_DataInit(d));INIT_STEP(VL53L0X_StaticInit(d));
 uint32_t count;uint8_t aperture,vhv,phase;
 INIT_STEP(VL53L0X_PerformRefSpadManagement(d,&count,&aperture));
 INIT_STEP(VL53L0X_PerformRefCalibration(d,&vhv,&phase));
 INIT_STEP(VL53L0X_SetDeviceMode(d,VL53L0X_DEVICEMODE_SINGLE_RANGING));
 INIT_STEP(VL53L0X_SetMeasurementTimingBudgetMicroSeconds(d,50000));
 uint32_t budget=0;INIT_STEP(VL53L0X_GetMeasurementTimingBudgetMicroSeconds(d,&budget));
 printf("BUDGET_READBACK %lu\n",(unsigned long)budget);
 uint8_t seq=0;INIT_STEP(VL53L0X_RdByte(d,1,&seq));printf("SEQUENCE 0x%02x\n",seq);
 for(unsigned long i=0;;i++){
  int start=VL53L0X_StartMeasurement(d);if(start){printf("FAIL start=%d state=FAULT\n",start);break;}
  uint8_t ready=0;int e=0;int64_t deadline=esp_timer_get_time()+1000000;
  while(!ready&&!e&&esp_timer_get_time()<deadline){e=VL53L0X_GetMeasurementDataReady(d,&ready);vTaskDelay(1);}
  if(e||!ready){printf("FAIL measurement error=%d ready=%u\n",e,ready);break;}
  VL53L0X_RangingMeasurementData_t m={0};e=VL53L0X_GetRangingMeasurementData(d,&m);
  if(e){printf("FAIL read=%d\n",e);break;}
  if(m.RangeStatus==0){
   printf("RANGE sample=%lu t_ms=%lld mm=%u raw_mm=%u status=0 valid=1 state=VALID\n",i,(long long)(esp_timer_get_time()/1000),m.RangeMilliMeter,m.RangeMilliMeter);
  }else{
   printf("RANGE sample=%lu t_ms=%lld mm=NA raw_mm=%u status=%u valid=0 state=INVALID\n",i,(long long)(esp_timer_get_time()/1000),m.RangeMilliMeter,m.RangeStatus);
  }
  if(VL53L0X_ClearInterruptMask(d,0)){printf("FAIL clear interrupt\n");break;}
  vTaskDelay(pdMS_TO_TICKS(50));
 }
 int stop=VL53L0X_StopMeasurement(d);printf("DONE stop=%d\n",stop);
}
