# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13...3.27)

set(PICO_BOARD pico CACHE STRING "Board type")

set(PROGRAM_NAME usb-ducky)

# Pull in Pico SDK (must be before project)
# If PICO_SDK_PATH isn't provided, fetch from git so the
# example can build without a preinstalled SDK.
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_FETCH_FROM_GIT ON)
endif()
include(pico_sdk_import.cmake)

project(${PROGRAM_NAME})

pico_sdk_init()

add_subdirectory(../../src build)

add_executable(${PROGRAM_NAME}
  hw_config.c
  main.c
  msc_disk.c
  usb_descriptors.c
)
target_link_libraries(${PROGRAM_NAME} PRIVATE
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    pico_stdlib
    tinyusb_additions
    tinyusb_board
    tinyusb_device
)

target_include_directories(${PROGRAM_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
)
