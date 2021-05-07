include_directories("${PROJECT_SOURCE_DIR}/pico_tuya/tuya_mcu_sdk")

set(APP_SRC main.cpp)

aux_source_directory(${PROJECT_SOURCE_DIR}/pico_tuya/tuya_mcu_sdk MCU_SDK_SRC)

add_executable(pico_tuya ${APP_SRC} ${MCU_SDK_SRC})

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(pico_tuya pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(pico_tuya)

# add url via pico_set_program_url
example_auto_set_url(pico_tuya)
