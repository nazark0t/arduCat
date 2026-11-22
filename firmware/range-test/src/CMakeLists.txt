file(GLOB ST_SOURCES "../vendor/sensor/vl53l0x/api/core/src/*.c")
idf_component_register(SRCS "main.c" "platform.c" ${ST_SOURCES} INCLUDE_DIRS "." "../vendor/sensor/vl53l0x/api/core/inc" REQUIRES esp_driver_i2c)
target_compile_definitions(${COMPONENT_LIB} PRIVATE USE_I2C_2V8)
