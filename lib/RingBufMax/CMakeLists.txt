set(TARGET_NAME "ringbufmax")

if (NOT TARGET ${TARGET_NAME})
    add_library(${TARGET_NAME} INTERFACE)

    target_sources(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/RingBufMax.c
    )
    
    target_include_directories(${TARGET_NAME} INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}
    )

    target_link_libraries(${TARGET_NAME} INTERFACE
        pico_stdlib
    )
endif()