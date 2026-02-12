# CMakeLists.txt for LwPKT Unit Tests (PC Build)
# Author: TDD Agent (c-pro mode)
# Date: 2026-02-09

cmake_minimum_required(VERSION 3.15)
project(LgLwPktTests C)

# Compiler settings (PC build, not embedded)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../leather_gauge_sensor/interfaces
    ${CMAKE_SOURCE_DIR}/../leather_gauge_sensor/core
    ${CMAKE_SOURCE_DIR}/../Third_Party/lwpkt/src/include
    ${CMAKE_SOURCE_DIR}/../Third_Party/lwrb/src/include
)

# Unity testing framework (assuming it's available, otherwise download)
# For simplicity, assume Unity is in Third_Party or system path
# TODO: Add Unity as git submodule or download script

# Source files for codec implementation
set(CODEC_SOURCES
    ${CMAKE_SOURCE_DIR}/../leather_gauge_sensor/adapters/comms_lwpkt/lg_lwpkt_codec.c
    ${CMAKE_SOURCE_DIR}/../Third_Party/lwpkt/src/lwpkt/lwpkt.c
    ${CMAKE_SOURCE_DIR}/../Third_Party/lwrb/src/lwrb/lwrb.c
)

# Test executable
add_executable(test_lwpkt_codec
    test_lwpkt_codec.c
    ${CODEC_SOURCES}
    # Add Unity sources here when available
    # ${CMAKE_SOURCE_DIR}/../Third_Party/Unity/src/unity.c
)

# Enable testing
enable_testing()
add_test(NAME LwPktCodecTests COMMAND test_lwpkt_codec)

# Build instructions (for agents/users):
# mkdir -p build && cd build
# cmake ..
# cmake --build .
# ctest --output-on-failure
