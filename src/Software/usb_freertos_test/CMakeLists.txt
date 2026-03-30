cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(PICO_BOARD pico2 CACHE STRING "Board type")

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS-Kernel)
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

project(usb_freertos_cdc C CXX ASM)

pico_sdk_init()

add_executable(usb_freertos_cdc
    main.c
    usb_descriptors.c
)

pico_set_program_name(usb_freertos_cdc "USB FreeRTOS CDC")
pico_set_program_version(usb_freertos_cdc "1.0")

pico_enable_stdio_uart(usb_freertos_cdc 0)
pico_enable_stdio_usb(usb_freertos_cdc 0)

target_include_directories(usb_freertos_cdc PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usb_freertos_cdc
    pico_stdlib
    hardware_clocks
    hardware_irq
    FreeRTOS-Kernel-Heap4
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(usb_freertos_cdc)