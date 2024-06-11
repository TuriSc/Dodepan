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
add_subdirectory(lib/RP2040-pico_synth_ex pico_synth_ex_i2s)
add_subdirectory(lib/RP2040-Button button)
add_subdirectory(lib/rpi-pico-mpu6050 haw_mpu6050)
add_subdirectory(lib/RP2040-Battery-Check battery_check)
add_subdirectory(lib/RP2040-Rotary-Encoder)

target_sources(${PROJECT_NAME} PRIVATE
        # ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/imu.c
        ${CMAKE_CURRENT_LIST_DIR}/touch.c
        ${CMAKE_CURRENT_LIST_DIR}/display/display.c
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306/ssd1306.c
        )

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/rpi-pico-mpu6050/include/haw
        # ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb-midi
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE
        # Define only one audio output
        # USE_AUDIO_PWM=1
        USE_AUDIO_I2S=1
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        hardware_i2c
        hardware_dma
        hardware_adc
        pico-mpr121
        pico_synth_ex_i2s
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