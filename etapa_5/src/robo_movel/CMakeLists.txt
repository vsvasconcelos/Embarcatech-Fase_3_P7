# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(robo_movel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# BLE configuration defines
add_compile_definitions(
    MAX_NR_HCI_CONNECTIONS=2
    MAX_NR_GATT_CLIENTS=2
    MAX_NR_L2CAP_SERVICES=2
    MAX_NR_L2CAP_CHANNELS=2
    MAX_NR_SM_LOOKUP_ENTRIES=2
    HCI_ACL_PAYLOAD_SIZE=512
    MAX_ATT_DB_SIZE=256
)

# Add executable
add_executable(robo_movel 
    main.c
    led.c
    motor.c
    sensor.c
    ble_server.c
)

pico_set_program_name(robo_movel "robo_movel")
pico_set_program_version(robo_movel "1.0")

# Generate GATT header from .gatt file
pico_btstack_make_gatt_header(robo_movel PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/robo.gatt")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(robo_movel 0)
pico_enable_stdio_usb(robo_movel 1)

# Add libraries
target_link_libraries(robo_movel
    pico_stdlib
    hardware_i2c
    hardware_pwm
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
)

# Add include directories
target_include_directories(robo_movel PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(robo_movel)
