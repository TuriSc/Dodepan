cmake_minimum_required(VERSION 3.13)
 
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
 
project(Dodepan C CXX ASM)
 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
 
pico_sdk_init()

set(LIBS
        lib/RP2040-Button/button.c
        lib/RingBufMax/RingBufMax.c
        lib/tinyusb-midi/usb_descriptors.c
        )

add_executable(${PROJECT_NAME}
        main.c
        )

add_subdirectory(lib/pico-mpr121/lib mpr121)
add_subdirectory(lib/RP2040-I2S-Audio-Mixer i2s_audio_mixer)

target_sources(${PROJECT_NAME} PRIVATE
        scales.h
        ${LIBS}
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        lib/RP2040-Button
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
        hardware_adc
        pico-mpr121
        i2s_audio_mixer
        tinyusb_device
        tinyusb_board
        pico_unique_id
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_AUDIO_I2S_MONO_INPUT=1
        PICO_AUDIO_I2S_DATA_PIN=0
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=1
        )

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} "1.0.0")

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
