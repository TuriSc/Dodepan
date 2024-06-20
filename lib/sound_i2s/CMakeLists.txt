set(TARGET_NAME "sound_i2s_16bits")

if (NOT TARGET ${TARGET_NAME})
    add_library(${TARGET_NAME} INTERFACE)

    target_sources(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/sound_i2s.c
    )

    pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/sound_i2s_16bits.pio)

    target_include_directories(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}
    )

    target_link_libraries(${TARGET_NAME} INTERFACE
        pico_stdlib
        hardware_dma
        hardware_i2c
        hardware_irq
        hardware_pio
    )
endif()
