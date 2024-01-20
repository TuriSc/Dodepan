cmake_minimum_required(VERSION 3.13)
 
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
 
project(Dodepan C CXX ASM)
 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
 
pico_sdk_init()

add_executable(${PROJECT_NAME}
        main.c
)

add_subdirectory(lib/pico-mpr121/lib mpr121)
add_subdirectory(lib/RP2040-I2S-Audio-Mixer i2s_audio_mixer)
add_subdirectory(lib/RP2040-Button button)
add_subdirectory(lib/rpi-pico-mpu6050 haw_mpu6050)
add_subdirectory(lib/RingBufMax ringbufmax)
add_subdirectory(lib/RP2040-Battery-Check battery_check)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/imu.c
        ${CMAKE_CURRENT_LIST_DIR}/demuxer.c
        ${CMAKE_CURRENT_LIST_DIR}/touch.c
        )

target_include_directories(${PROJECT_NAME} PRIVATE
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/rpi-pico-mpu6050/include/haw
        ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi
        ${CMAKE_CURRENT_LIST_DIR}/lib/RingBufMax
        )

set_target_properties(${PROJECT_NAME} PROPERTIES PICO_TARGET_LINKER_SCRIPT 
    ${CMAKE_SOURCE_DIR}/memmap_custom.ld)

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
        button
        haw_mpu6050
        ringbufmax
        battery_check
        )

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} "1.0.0")

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
