cmake_minimum_required(VERSION 3.13)
 
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
 
project(Dodepan C CXX ASM)
 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
 
pico_sdk_init()

add_executable(${PROJECT_NAME}
        main.cpp
)

add_subdirectory(lib/pico-mpr121/lib mpr121)
add_subdirectory(lib/RP2040-Button button)
add_subdirectory(lib/rpi-pico-mpu6050 haw_mpu6050)
add_subdirectory(lib/RP2040-Battery-Check battery_check)
add_subdirectory(lib/RP2040-Rotary-Encoder)
add_subdirectory(lib/sound_i2s)

target_sources(${PROJECT_NAME} PRIVATE
        # ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/imu.c
        ${CMAKE_CURRENT_LIST_DIR}/state.c
        ${CMAKE_CURRENT_LIST_DIR}/touch.c
        ${CMAKE_CURRENT_LIST_DIR}/looper.c
        ${CMAKE_CURRENT_LIST_DIR}/display/display.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306/ssd1306.c
        )

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/rpi-pico-mpu6050/include/haw
        # ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306
        ${CMAKE_CURRENT_LIST_DIR}/lib/digital-synth-pra32-u/Digital-Synth-PRA32-U
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        hardware_i2c
        hardware_dma
        hardware_adc
        hardware_flash
        pico-mpr121
        pico_multicore
        sound_i2s_16bits
        # tinyusb_device
        # tinyusb_board
        pico_unique_id
        button
        haw_mpu6050
        battery_check
        pico_rotary_encoder
        )

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
