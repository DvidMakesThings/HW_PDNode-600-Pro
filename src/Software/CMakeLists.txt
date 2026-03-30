cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# FreeRTOS kernel path (raspberrypi/FreeRTOS-Kernel submodule)
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS RP2350 Cortex-M33 NTZ port (must be after SDK import, before project())
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

project(PDNode_RTOS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# --------------------------------------------------------------------------
# Source files
# --------------------------------------------------------------------------
set(PDNode_SOURCES
    PDNode_RTOS.c

    # Tasks
    tasks/LoggerTask.c
    tasks/ConsoleTask.c
    tasks/HealthTask.c
    tasks/StorageTask.c
    tasks/NetTask.c
    tasks/PDCardTask.c
    tasks/USBATask.c
    tasks/HeartbeatTask.c
    tasks/InitTask.c

    # Drivers
    drivers/i2c_bus.c
    drivers/CAT24C256_driver.c
    drivers/MCP23017_driver.c
    drivers/PAC1720_driver.c
    drivers/TCA9548_driver.c
    drivers/ethernet_driver.c
    drivers/socket.c
    drivers/snmp.c
    drivers/snmp_table.c

    # EEPROM I/O layer
    eeprom/eeprom_io.c

    # Web
    web/http_server.c
    web/status_handler.c

    # USB descriptors (TinyUSB device)
    usb_descriptors.c
)

add_executable(PDNode_RTOS ${PDNode_SOURCES})

pico_set_program_name(PDNode_RTOS "PDNode-600 Pro")
pico_set_program_version(PDNode_RTOS "1.0.0")

# Direct TinyUSB under FreeRTOS — no pico_stdio_usb
pico_enable_stdio_uart(PDNode_RTOS 0)
pico_enable_stdio_usb(PDNode_RTOS 0)

# --------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------
target_include_directories(PDNode_RTOS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/drivers
    ${CMAKE_CURRENT_LIST_DIR}/eeprom
    ${CMAKE_CURRENT_LIST_DIR}/tasks
    ${CMAKE_CURRENT_LIST_DIR}/web
)

# --------------------------------------------------------------------------
# Link libraries
# --------------------------------------------------------------------------
target_link_libraries(PDNode_RTOS
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_gpio
    hardware_pwm
    hardware_adc
    hardware_watchdog
    hardware_clocks
    hardware_resets
    hardware_timer
    pico_unique_id
    FreeRTOS-Kernel-Heap4
    tinyusb_device
    tinyusb_board
)


# --------------------------------------------------------------------------
# Build outputs (UF2, bin, hex, dis)
# --------------------------------------------------------------------------
pico_add_extra_outputs(PDNode_RTOS)
