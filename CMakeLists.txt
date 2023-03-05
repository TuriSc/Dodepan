cmake_minimum_required(VERSION 3.13)
 
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
 
project(Dodepan C CXX ASM)
 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
 
pico_sdk_init()

set(LIBS
        lib/RP2040-Button/button.c
        lib/RP2040-PWM-DMA-Audio/audio.c
        lib/RP2040-Moving-Average/moving_average.c
        lib/rpi-pico-mpu6050/src/MPU6050.c
        lib/RingBufMax/RingBufMax.c
        lib/tinyusb-midi/usb_descriptors.c
        )

add_subdirectory(lib/pico-mpr121/lib mpr121)

add_executable(${PROJECT_NAME}
        main.c
        )

target_sources(${PROJECT_NAME} PRIVATE
        scales.h
        ${LIBS}
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        lib/RP2040-Button
        lib/RP2040-PWM-DMA-Audio
        lib/RP2040-Moving-Average
        lib/rpi-pico-mpu6050/include
        lib/RingBufMax
        )

target_include_directories(${PROJECT_NAME} PUBLIC
        lib/tinyusb-midi/
        .
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        hardware_i2c
        hardware_dma
        hardware_pwm
        hardware_adc
        pico-mpr121
        tinyusb_device
        tinyusb_board
        pico_unique_id
        )

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} "1.0.0")

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
