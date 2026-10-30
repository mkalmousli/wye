# Wye firmware: bridge (sender Pico) and target Pico.
# Built by `./build.c firmware`, which runs CMake because the pico-sdk requires it.
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(wye_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

set(WYE_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(WYE_FW_DIR ${CMAKE_CURRENT_LIST_DIR})
# Passed in by ./build.c (from VERSION and git).
set(WYE_VERSION "0.0.0" CACHE STRING "Firmware version x.y.z")
set(WYE_BUILD_ID "0" CACHE STRING "Short git hash (hex)")
string(REPLACE "." ";" _wye_parts "${WYE_VERSION}")
list(GET _wye_parts 0 WYE_VERSION_MAJOR)
list(GET _wye_parts 1 WYE_VERSION_MINOR)
list(GET _wye_parts 2 WYE_VERSION_PATCH)

file(GLOB WYE_PROTO_SRC ${WYE_ROOT}/proto/*.c)
set(WYE_SHARED_SRC
    ${WYE_FW_DIR}/shared/board.c
    ${WYE_FW_DIR}/shared/fw_flash.c
    ${WYE_FW_DIR}/shared/identity_store.c
    ${WYE_FW_DIR}/shared/uart_link.c
    ${WYE_FW_DIR}/shared/usb_strings.c
    ${WYE_FW_DIR}/shared/usb_audio.c
    ${WYE_FW_DIR}/shared/crash.c
    ${WYE_FW_DIR}/shared/usb_audio_ctrl.c
    ${WYE_ROOT}/devcore/audio_queue.c
    ${WYE_ROOT}/devcore/node.c
    ${WYE_ROOT}/devcore/node_maint.c
    ${WYE_ROOT}/devcore/node_ident.c
    ${WYE_ROOT}/devcore/fwupdate.c
    ${WYE_ROOT}/devcore/fwupdate_commit.c)

function(wye_firmware name)
    add_executable(${name} ${ARGN} ${WYE_SHARED_SRC} ${WYE_PROTO_SRC})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} ${WYE_ROOT}/proto ${WYE_ROOT}/devcore
        ${WYE_FW_DIR}/shared)
    target_compile_definitions(${name} PRIVATE
        WYE_VERSION_MAJOR=${WYE_VERSION_MAJOR} WYE_VERSION_MINOR=${WYE_VERSION_MINOR}
        WYE_VERSION_PATCH=${WYE_VERSION_PATCH} WYE_BUILD_ID=0x${WYE_BUILD_ID}
        PICO_PANIC_FUNCTION=wye_panic)  # restart and report instead of hanging (shared/crash.c)
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    target_link_libraries(${name} pico_stdlib hardware_uart hardware_flash hardware_watchdog
        tinyusb_device tinyusb_board)
    pico_add_extra_outputs(${name})
endfunction()

add_subdirectory(bridge)
add_subdirectory(target)
