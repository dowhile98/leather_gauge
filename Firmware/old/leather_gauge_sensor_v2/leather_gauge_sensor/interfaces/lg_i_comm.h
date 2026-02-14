#ifndef LG_I_COMM_H
#define LG_I_COMM_H

#include <stddef.h> // For NULL
#include "lg_domain_types.h"

/**
 * @brief Communication Interface (Port)
 * Defines the contract for the communication layer (LwPKT/RS-485).
 */
typedef struct lg_i_comm
{
    /**
     * @brief Initialize the communication hardware/stack
     * @param address Device address
     * @param baudrate Communication speed
     * @return lg_result_t
     */
    lg_result_t (*init)(uint8_t address, uint32_t baudrate);

    /**
     * @brief Process communication events (Receive/Send)
     * Should be called in the main loop.
     * @return lg_result_t
     */
    lg_result_t (*process)(void);

    /**
     * @brief Check if a packet has been received and retrieve it
     * @param packet Pointer to packet structure to fill
     * @return LG_OK if packet read, LG_BUSY/LG_ERROR if no packet
     */
    lg_result_t (*read)(lg_comm_packet_t *packet);

    /**
     * @brief Send a packet/response
     * @param cmd Command ID
     * @param data Payload data
     * @param len Payload length
     * @return lg_result_t
     */
    lg_result_t (*send)(uint8_t cmd, const void *data, uint16_t len);

    /**
     * @brief 🆕 Send a packet with FLAGS (for cascade reads)
     * @param cmd Command ID
     * @param flags FLAGS field (sensor# for cascade)
     * @param data Payload data
     * @param len Payload length
     * @return lg_result_t
     */
    lg_result_t (*send_with_flags)(uint8_t cmd, uint32_t flags, const void *data, uint16_t len);

    /**
     * @brief Update device address at runtime
     * @param address New address
     * @return lg_result_t
     */
    lg_result_t (*set_address)(uint8_t address);

} lg_i_comm_t;

/* ============================================================================
 * Inline Wrapper Functions (Enforce interface usage, prevent direct calls)
 * ========================================================================= */

/**
 * @brief Initialize communication interface (wrapper).
 * @note  Prevents direct function pointer calls, enforces NULL checks.
 */
static inline lg_result_t LgComm_Init(const lg_i_comm_t *iface, uint8_t address, uint32_t baudrate)
{
    if (iface == NULL || iface->init == NULL)
    {
        return LG_ERROR;
    }
    return iface->init(address, baudrate);
}

/**
 * @brief Process communication events (wrapper).
 */
static inline lg_result_t LgComm_Process(const lg_i_comm_t *iface)
{
    if (iface == NULL || iface->process == NULL)
    {
        return LG_ERROR;
    }
    return iface->process();
}

/**
 * @brief Read received packet (wrapper).
 */
static inline lg_result_t LgComm_Read(const lg_i_comm_t *iface, lg_comm_packet_t *packet)
{
    if (iface == NULL || iface->read == NULL || packet == NULL)
    {
        return LG_INVALID_PARAM;
    }
    return iface->read(packet);
}

/**
 * @brief Send packet/response (wrapper).
 */
static inline lg_result_t LgComm_Send(const lg_i_comm_t *iface, uint8_t cmd, const void *data, uint16_t len)
{
    if (iface == NULL || iface->send == NULL)
    {
        return LG_INVALID_PARAM;
    }
    return iface->send(cmd, data, len);
}

/**
 * @brief 🆕 Send packet with FLAGS (for cascade reads - wrapper).
 */
static inline lg_result_t LgComm_SendWithFlags(const lg_i_comm_t *iface, uint8_t cmd, uint32_t flags,
                                               const void *data, uint16_t len)
{
    if (iface == NULL || iface->send_with_flags == NULL)
    {
        return LG_INVALID_PARAM;
    }
    return iface->send_with_flags(cmd, flags, data, len);
}

/**
 * @brief Update device address (wrapper).
 */
static inline lg_result_t LgComm_SetAddress(const lg_i_comm_t *iface, uint8_t address)
{
    if (iface == NULL || iface->set_address == NULL)
    {
        return LG_ERROR;
    }
    return iface->set_address(address);
}

#endif // LG_I_COMM_H
