/**
 * @file    test_lwpkt_codec.c
 * @brief   Unit tests for LwPKT Codec Interface
 * @author  TDD Agent (c-pro mode)
 * @date    2026-02-09
 *
 * @note    These tests enforce TDD RED-GREEN-REFACTOR cycle.
 *          Run BEFORE implementing lg_lwpkt_codec.c
 */

#include "unity.h"
#include "lg_i_lwpkt.h"
#include "lg_domain_types.h"
#include <string.h>

/* ============================================================================
 * Test Fixtures
 * ========================================================================= */

/* Forward declaration of codec (to be implemented) */
extern ILwPktCodec_t *LgLwPktCodec_GetInterface(void);

static ILwPktCodec_t *codec;

void setUp(void)
{
    codec = LgLwPktCodec_GetInterface();
}

void tearDown(void)
{
    // No dynamic allocation, nothing to clean
}

/* ============================================================================
 * Test Cases - Encode
 * ========================================================================= */

/**
 * @test Encode_ValidCommand_NoPayload_ReturnsOK
 * @brief Verify encoding of command without payload succeeds.
 */
void test_Encode_ValidCommand_NoPayload_ReturnsOK(void)
{
    uint8_t buffer[32];
    uint16_t out_len;

    lg_result_t result = codec->Encode(
        1,               // from: sensor 1
        0xFF,            // to: master
        CMD_READ_SENSOR, // cmd
        NULL,            // no payload
        0,               // len = 0
        buffer,
        sizeof(buffer),
        &out_len);

    TEST_ASSERT_EQUAL(LG_OK, result);
    TEST_ASSERT_GREATER_THAN(0, out_len);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(buffer), out_len);
}

/**
 * @test Encode_ValidCommand_WithPayload_EncodesCorrectly
 * @brief Verify encoding with payload includes data and CRC.
 */
void test_Encode_ValidCommand_WithPayload_EncodesCorrectly(void)
{
    uint8_t buffer[64];
    uint16_t out_len;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD};

    lg_result_t result = codec->Encode(
        2,    // from: sensor 2
        0xFF, // to: master
        CMD_READ_SENSOR_RESP,
        payload,
        sizeof(payload),
        buffer,
        sizeof(buffer),
        &out_len);

    TEST_ASSERT_EQUAL(LG_OK, result);
    TEST_ASSERT_GREATER_THAN(sizeof(payload), out_len); // Must include overhead
}

/**
 * @test Encode_NullOutputBuffer_ReturnsInvalidParam
 * @brief Verify NULL output buffer is rejected.
 */
void test_Encode_NullOutputBuffer_ReturnsInvalidParam(void)
{
    uint16_t out_len;

    lg_result_t result = codec->Encode(
        1, 0xFF, CMD_READ_SENSOR,
        NULL, 0,
        NULL, // NULL buffer
        32,
        &out_len);

    TEST_ASSERT_EQUAL(LG_INVALID_PARAM, result);
}

/**
 * @test Encode_BufferTooSmall_ReturnsError
 * @brief Verify insufficient buffer space is detected.
 */
void test_Encode_BufferTooSmall_ReturnsError(void)
{
    uint8_t buffer[5]; // Too small for frame
    uint16_t out_len;
    uint8_t payload[10];

    lg_result_t result = codec->Encode(
        1, 2, CMD_WRITE_CONFIG,
        payload, sizeof(payload),
        buffer, sizeof(buffer), // Not enough space
        &out_len);

    TEST_ASSERT_EQUAL(LG_ERROR, result);
}

/* ============================================================================
 * Test Cases - Decode
 * ========================================================================= */

/**
 * @test Decode_ValidFrame_ExtractsFields
 * @brief Verify decoding of valid frame extracts all fields correctly.
 */
void test_Decode_ValidFrame_ExtractsFields(void)
{
    // First, encode a known frame
    uint8_t tx_buf[64];
    uint16_t tx_len;
    uint8_t test_payload[] = {0x12, 0x34};

    codec->Encode(3, 0xFF, CMD_READ_RAW, test_payload, sizeof(test_payload),
                  tx_buf, sizeof(tx_buf), &tx_len);

    // Now decode it
    uint8_t from, to, cmd;
    const uint8_t *rx_payload;
    uint16_t rx_len;

    lg_result_t result = codec->Decode(
        tx_buf, tx_len,
        &from, &to, &cmd,
        &rx_payload, &rx_len);

    TEST_ASSERT_EQUAL(LG_OK, result);
    TEST_ASSERT_EQUAL(3, from);
    TEST_ASSERT_EQUAL(0xFF, to);
    TEST_ASSERT_EQUAL(CMD_READ_RAW, cmd);
    TEST_ASSERT_EQUAL(sizeof(test_payload), rx_len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(test_payload, rx_payload, rx_len);
}

/**
 * @test Decode_CorruptedCRC_ReturnsError
 * @brief Verify corrupted CRC is detected and rejected.
 */
void test_Decode_CorruptedCRC_ReturnsError(void)
{
    // Encode valid frame
    uint8_t frame[32];
    uint16_t frame_len;

    codec->Encode(1, 2, CMD_GET_STATUS, NULL, 0,
                  frame, sizeof(frame), &frame_len);

    // Corrupt the last byte (assuming it's CRC or STOP)
    frame[frame_len - 2] ^= 0xFF;

    // Try to decode
    uint8_t from, to, cmd;
    const uint8_t *payload;
    uint16_t len;

    lg_result_t result = codec->Decode(
        frame, frame_len,
        &from, &to, &cmd,
        &payload, &len);

    TEST_ASSERT_EQUAL(LG_ERROR, result);
}

/**
 * @test Decode_NullFrame_ReturnsInvalidParam
 * @brief Verify NULL input is rejected.
 */
void test_Decode_NullFrame_ReturnsInvalidParam(void)
{
    uint8_t from, to, cmd;
    const uint8_t *payload;
    uint16_t len;

    lg_result_t result = codec->Decode(
        NULL, 10, // NULL frame
        &from, &to, &cmd,
        &payload, &len);

    TEST_ASSERT_EQUAL(LG_INVALID_PARAM, result);
}

/**
 * @test Decode_ZeroLength_ReturnsInvalidParam
 * @brief Verify zero-length frame is rejected.
 */
void test_Decode_ZeroLength_ReturnsInvalidParam(void)
{
    uint8_t frame[32] = {0};
    uint8_t from, to, cmd;
    const uint8_t *payload;
    uint16_t len;

    lg_result_t result = codec->Decode(
        frame, 0, // Zero length
        &from, &to, &cmd,
        &payload, &len);

    TEST_ASSERT_EQUAL(LG_INVALID_PARAM, result);
}

/* ============================================================================
 * Test Cases - Round-Trip
 * ========================================================================= */

/**
 * @test RoundTrip_EncodeDecodeMultipleCommands_Consistent
 * @brief Verify encode->decode produces identical data for all commands.
 */
void test_RoundTrip_EncodeDecodeMultipleCommands_Consistent(void)
{
    uint8_t commands[] = {
        CMD_READ_SENSOR,
        CMD_WRITE_CONFIG,
        CMD_CALIBRATE,
        CMD_GET_STATUS,
        CMD_ERROR};

    for (uint8_t i = 0; i < sizeof(commands); i++)
    {
        uint8_t tx_buf[64];
        uint16_t tx_len;
        uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

        // Encode
        lg_result_t enc_res = codec->Encode(
            i + 1, 0xFF, commands[i],
            test_data, sizeof(test_data),
            tx_buf, sizeof(tx_buf), &tx_len);
        TEST_ASSERT_EQUAL_MESSAGE(LG_OK, enc_res, "Encode failed");

        // Decode
        uint8_t from, to, cmd;
        const uint8_t *rx_data;
        uint16_t rx_len;

        lg_result_t dec_res = codec->Decode(
            tx_buf, tx_len,
            &from, &to, &cmd,
            &rx_data, &rx_len);
        TEST_ASSERT_EQUAL_MESSAGE(LG_OK, dec_res, "Decode failed");

        // Verify
        TEST_ASSERT_EQUAL(i + 1, from);
        TEST_ASSERT_EQUAL(commands[i], cmd);
        TEST_ASSERT_EQUAL(sizeof(test_data), rx_len);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data, rx_data, rx_len);
    }
}

/* ============================================================================
 * Test Runner
 * ========================================================================= */

int main(void)
{
    UNITY_BEGIN();

    // Encode tests
    RUN_TEST(test_Encode_ValidCommand_NoPayload_ReturnsOK);
    RUN_TEST(test_Encode_ValidCommand_WithPayload_EncodesCorrectly);
    RUN_TEST(test_Encode_NullOutputBuffer_ReturnsInvalidParam);
    RUN_TEST(test_Encode_BufferTooSmall_ReturnsError);

    // Decode tests
    RUN_TEST(test_Decode_ValidFrame_ExtractsFields);
    RUN_TEST(test_Decode_CorruptedCRC_ReturnsError);
    RUN_TEST(test_Decode_NullFrame_ReturnsInvalidParam);
    RUN_TEST(test_Decode_ZeroLength_ReturnsInvalidParam);

    // Round-trip tests
    RUN_TEST(test_RoundTrip_EncodeDecodeMultipleCommands_Consistent);

    return UNITY_END();
}
