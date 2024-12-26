cmake_minimum_required(VERSION 3.13)

set(TARGET i2c)

# Enable debug to make debugging faster
set(CMAKE_BUILD_TYPE Debug)

include(pico_sdk_import.cmake)

project(${TARGET}_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

# Generate compile_commands.json for our LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set( SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src )
set( INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include )
file(GLOB SRC_FILES
    "${SRC_DIR}/*.c"
)
# file(GLOB HEADER_FILES
#     "${INCLUDE_DIR}/*.c"
# )
# source_group("Header Files" FILES ${HEADER_FILES})
include_directories(${INCLUDE_DIR})

add_executable(${TARGET}
    ${SRC_FILES}
)

# Enable stdio via UART
pico_enable_stdio_usb(${TARGET} 0)
pico_enable_stdio_uart(${TARGET} 1)

pico_add_extra_outputs(${TARGET})

target_link_libraries(${TARGET} pico_stdlib hardware_i2c pico_i2c_slave)
