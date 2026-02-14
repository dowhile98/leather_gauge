# CMakeLists.txt for Leather Gauge Controller Unit Tests
# Uses Unity test framework

cmake_minimum_required(VERSION 3.14)
project(leather_gauge_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ============================================================================
# Unity Test Framework
# ============================================================================
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.5.2
)
FetchContent_MakeAvailable(unity)

# ============================================================================
# Project Paths
# ============================================================================
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LGC_ROOT ${PROJECT_ROOT}/leather_gauge_controller)

# ============================================================================
# Include Directories
# ============================================================================
include_directories(
    ${LGC_ROOT}/domain/interfaces
    ${LGC_ROOT}/adapters/sensor_cache
    ${LGC_ROOT}/app/inc
    ${LGC_ROOT}/osal/include
    ${LGC_ROOT}/osal/common
    ${LGC_ROOT}/config
    ${unity_SOURCE_DIR}/src
)

# ============================================================================
# Test-specific defines (disable RTOS for unit tests)
# ============================================================================
add_compile_definitions(
    # Disable RTOS mutex/semaphore for unit tests
    # USE_RTOS
    
    # Test configuration
    LGC_SENSOR_NUMBER=11
)

# ============================================================================
# Mock OSAL for tests (stub implementations)
# ============================================================================
set(MOCK_OSAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/os_port_mock.c
)

# ============================================================================
# Source Files Under Test
# ============================================================================
set(SENSOR_CACHE_SOURCES
    ${LGC_ROOT}/adapters/sensor_cache/lgc_sensor_cache.c
)

# ============================================================================
# Test Executables
# ============================================================================

# Test: Sensor Cache
add_executable(test_sensor_cache
    unit/test_lgc_sensor_cache.c
    ${SENSOR_CACHE_SOURCES}
    ${MOCK_OSAL_SOURCES}
)
target_link_libraries(test_sensor_cache unity)

# ============================================================================
# CTest Integration
# ============================================================================
enable_testing()

add_test(NAME SensorCacheTests COMMAND test_sensor_cache)

# ============================================================================
# Custom Targets
# ============================================================================
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_sensor_cache
    COMMENT "Running all unit tests"
)
