/**
 * @file test_lgc_sensor_cache.c
 * @brief Unit tests for Sensor Cache implementation
 *
 * TDD Red-Green-Refactor cycle tests for LgcSensorCache adapter.
 *
 * @date Created: Feb 13, 2026
 * @author GitHub Copilot
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "unity.h"
#include "lgc_sensor_cache.h"
#include "lgc_i_sensor_cache.h"

/* ============================================================================
 * TEST FIXTURES
 * ============================================================================ */
static LgcSensorCache_t s_cache;
static ILgcSensorCache_t *s_iface;

void setUp(void)
{
    /* Initialize cache before each test */
    LgcSensorCache_Init(&s_cache);
    s_iface = LgcSensorCache_GetInterface(&s_cache);
}

void tearDown(void)
{
    /* Cleanup after each test */
}

/* ============================================================================
 * TEST CASES: Initialization
 * ============================================================================ */

/**
 * @brief Test: Init should return ERR_OK with valid pointer
 */
void test_Init_ValidPointer_ReturnsOk(void)
{
    LgcSensorCache_t cache;
    error_t result = LgcSensorCache_Init(&cache);

    TEST_ASSERT_EQUAL(NO_ERROR, result);
}

/**
 * @brief Test: Init with NULL pointer should return error
 */
void test_Init_NullPointer_ReturnsError(void)
{
    error_t result = LgcSensorCache_Init(NULL);

    TEST_ASSERT_EQUAL(ERROR_INVALID_PARAMETER, result);
}

/**
 * @brief Test: After init, all sensors should have zero values
 */
void test_Init_SensorsStartAtZero(void)
{
    LgcSensorArray_t array;
    s_iface->get_all_sensors(s_iface->context, &array);

    for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
    {
        TEST_ASSERT_EQUAL_UINT16(0, array.sensors[i].value);
    }
}

/**
 * @brief Test: After init, sequence number should be zero
 */
void test_Init_SequenceStartsAtZero(void)
{
    uint32_t seq = s_iface->get_sequence(s_iface->context);

    TEST_ASSERT_EQUAL_UINT32(0, seq);
}

/* ============================================================================
 * TEST CASES: Update Operations
 * ============================================================================ */

/**
 * @brief Test: Update single sensor should change its value
 */
void test_UpdateSensor_ValidId_UpdatesValue(void)
{
    const uint8_t sensor_id = 5;
    const uint16_t test_value = 0x03FF; /* All 10 bits active */
    const uint32_t timestamp = 1000;

    error_t result = LgcSensorCache_UpdateSensor(
        &s_cache, sensor_id, test_value, LGC_SENSOR_HEALTHY, timestamp);

    TEST_ASSERT_EQUAL(NO_ERROR, result);

    LgcSensorData_t sensor_data;
    s_iface->get_sensor(s_iface->context, sensor_id, &sensor_data);

    TEST_ASSERT_EQUAL_UINT16(test_value, sensor_data.value);
    TEST_ASSERT_EQUAL(LGC_SENSOR_HEALTHY, sensor_data.status);
    TEST_ASSERT_EQUAL_UINT32(timestamp, sensor_data.timestamp_ms);
}

/**
 * @brief Test: Update with invalid sensor ID should return error
 */
void test_UpdateSensor_InvalidId_ReturnsError(void)
{
    error_t result = LgcSensorCache_UpdateSensor(
        &s_cache, LGC_SENSOR_NUMBER, 0x0000, LGC_SENSOR_HEALTHY, 0);

    TEST_ASSERT_EQUAL(ERROR_INVALID_PARAMETER, result);
}

/**
 * @brief Test: Update should increment sequence number
 */
void test_UpdateSensor_IncrementsSequence(void)
{
    uint32_t seq_before = s_iface->get_sequence(s_iface->context);

    LgcSensorCache_UpdateSensor(&s_cache, 0, 0x0001, LGC_SENSOR_HEALTHY, 100);

    uint32_t seq_after = s_iface->get_sequence(s_iface->context);

    TEST_ASSERT_EQUAL_UINT32(seq_before + 1, seq_after);
}

/**
 * @brief Test: Update with timeout status should set fault bit
 */
void test_UpdateSensor_TimeoutStatus_SetsFaultBit(void)
{
    const uint8_t sensor_id = 3;

    LgcSensorCache_UpdateSensor(
        &s_cache, sensor_id, 0, LGC_SENSOR_TIMEOUT, 100);

    uint16_t status_mask = s_iface->get_status_mask(s_iface->context);

    TEST_ASSERT_TRUE(status_mask & (1 << sensor_id));
}

/* ============================================================================
 * TEST CASES: Read Operations
 * ============================================================================ */

/**
 * @brief Test: Get all sensors should return complete snapshot
 */
void test_GetAllSensors_ReturnsCompleteArray(void)
{
    /* Setup: Update all sensors with known values */
    for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
    {
        LgcSensorCache_UpdateSensor(&s_cache, i, (i + 1) * 100, LGC_SENSOR_HEALTHY, i * 10);
    }

    LgcSensorArray_t array;
    error_t result = s_iface->get_all_sensors(s_iface->context, &array);

    TEST_ASSERT_EQUAL(NO_ERROR, result);

    for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
    {
        TEST_ASSERT_EQUAL_UINT16((i + 1) * 100, array.sensors[i].value);
    }
}

/**
 * @brief Test: Get all sensors with NULL output should return error
 */
void test_GetAllSensors_NullOutput_ReturnsError(void)
{
    error_t result = s_iface->get_all_sensors(s_iface->context, NULL);

    TEST_ASSERT_EQUAL(ERROR_INVALID_PARAMETER, result);
}

/**
 * @brief Test: Get single sensor with invalid ID returns error
 */
void test_GetSensor_InvalidId_ReturnsError(void)
{
    LgcSensorData_t data;
    error_t result = s_iface->get_sensor(s_iface->context, LGC_SENSOR_NUMBER, &data);

    TEST_ASSERT_EQUAL(ERROR_INVALID_PARAMETER, result);
}

/* ============================================================================
 * TEST CASES: Health Status
 * ============================================================================ */

/**
 * @brief Test: All healthy after init
 */
void test_IsAllHealthy_AfterInit_ReturnsTrue(void)
{
    /* Init sets all to HEALTHY by default */
    bool healthy = s_iface->is_all_healthy(s_iface->context);

    TEST_ASSERT_TRUE(healthy);
}

/**
 * @brief Test: Not healthy if one sensor has fault
 */
void test_IsAllHealthy_OneFault_ReturnsFalse(void)
{
    LgcSensorCache_UpdateSensor(&s_cache, 7, 0, LGC_SENSOR_TIMEOUT, 100);

    bool healthy = s_iface->is_all_healthy(s_iface->context);

    TEST_ASSERT_FALSE(healthy);
}

/**
 * @brief Test: Healthy again after fault cleared
 */
void test_IsAllHealthy_FaultCleared_ReturnsTrue(void)
{
    /* First: set fault */
    LgcSensorCache_UpdateSensor(&s_cache, 7, 0, LGC_SENSOR_TIMEOUT, 100);
    TEST_ASSERT_FALSE(s_iface->is_all_healthy(s_iface->context));

    /* Then: clear fault */
    LgcSensorCache_UpdateSensor(&s_cache, 7, 0x0100, LGC_SENSOR_HEALTHY, 200);
    TEST_ASSERT_TRUE(s_iface->is_all_healthy(s_iface->context));
}

/* ============================================================================
 * TEST CASES: Cycle Time Tracking
 * ============================================================================ */

/**
 * @brief Test: Begin/End cycle updates timing
 */
void test_CycleTracking_RecordsCycleTime(void)
{
    LgcSensorCache_BeginCycle(&s_cache, 1000);

    /* Simulate reads */
    for (uint8_t i = 0; i < LGC_SENSOR_NUMBER; i++)
    {
        LgcSensorCache_UpdateSensor(&s_cache, i, 0, LGC_SENSOR_HEALTHY, 1000 + i * 10);
    }

    LgcSensorCache_EndCycle(&s_cache, 1110);

    LgcSensorArray_t array;
    s_iface->get_all_sensors(s_iface->context, &array);

    TEST_ASSERT_EQUAL_UINT32(110, array.read_cycle_ms);
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(void)
{
    UNITY_BEGIN();

    /* Initialization tests */
    RUN_TEST(test_Init_ValidPointer_ReturnsOk);
    RUN_TEST(test_Init_NullPointer_ReturnsError);
    RUN_TEST(test_Init_SensorsStartAtZero);
    RUN_TEST(test_Init_SequenceStartsAtZero);

    /* Update tests */
    RUN_TEST(test_UpdateSensor_ValidId_UpdatesValue);
    RUN_TEST(test_UpdateSensor_InvalidId_ReturnsError);
    RUN_TEST(test_UpdateSensor_IncrementsSequence);
    RUN_TEST(test_UpdateSensor_TimeoutStatus_SetsFaultBit);

    /* Read tests */
    RUN_TEST(test_GetAllSensors_ReturnsCompleteArray);
    RUN_TEST(test_GetAllSensors_NullOutput_ReturnsError);
    RUN_TEST(test_GetSensor_InvalidId_ReturnsError);

    /* Health status tests */
    RUN_TEST(test_IsAllHealthy_AfterInit_ReturnsTrue);
    RUN_TEST(test_IsAllHealthy_OneFault_ReturnsFalse);
    RUN_TEST(test_IsAllHealthy_FaultCleared_ReturnsTrue);

    /* Cycle tracking tests */
    RUN_TEST(test_CycleTracking_RecordsCycleTime);

    return UNITY_END();
}
