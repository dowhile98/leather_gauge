/**
 * @file error.h
 * @brief Error codes stub for unit tests
 */

#ifndef TEST_ERROR_H
#define TEST_ERROR_H

typedef enum
{
    NO_ERROR = 0,
    ERROR_FAILURE = 1,
    ERROR_INVALID_PARAMETER,
    ERROR_TIMEOUT,
    ERROR_BAD_CRC,
} error_t;

#endif /* TEST_ERROR_H */
