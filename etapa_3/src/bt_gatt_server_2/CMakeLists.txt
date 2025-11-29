cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picow_ble_temp_sensor C CXX ASM)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(${PROJECT_NAME}
    server.c
    )

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )

# Pull in Raspberry Pi Pico SDK (must be before project)
#include(pico_sdk_import.cmake)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # For our common btstack config
    )
pico_btstack_make_gatt_header(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

#example_auto_set_url(${PROJECT_NAME})
