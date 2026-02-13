#include "lg_core.h"
#include <string.h>

/* ============================================================================
 * Private Data Types
 * ========================================================================= */
typedef struct
{
    const lg_i_comm_t *comm;
    const lg_i_sensor_t *sensor;
    const lg_i_storage_t *storage;

    lg_config_t config;
    lg_sensor_data_t current_data;
    lg_comm_packet_t rx_packet;
} core_context_t;

/* ============================================================================
 * Private Variables
 * ========================================================================= */
static core_context_t ctx;

/* ============================================================================
 * Private Function Prototypes
 * ========================================================================= */
static void handle_command(void);

/* ============================================================================
 * Public Functions
 * ========================================================================= */
lg_result_t lg_core_init(const lg_i_comm_t *comm,
                         const lg_i_sensor_t *sensor,
                         const lg_i_storage_t *storage)
{
    if (!comm || !sensor || !storage)
        return LG_INVALID_PARAM;

    ctx.comm = comm;
    ctx.sensor = sensor;
    ctx.storage = storage;

    // 1. Init Storage & Load Config
    if (ctx.storage->init() != LG_OK)
        return LG_ERROR;
    if (ctx.storage->load_config(&ctx.config) != LG_OK)
        return LG_ERROR;

    // 2. Init Sensor
    if (ctx.sensor->init(ctx.config.fc) != LG_OK)
        return LG_ERROR;

    // 3. Init Comms
    if (LgComm_Init(ctx.comm, ctx.config.address, ctx.config.baudrate) != LG_OK)
        return LG_ERROR;

    return LG_OK;
}

lg_result_t lg_core_run(void)
{
    // 1. Process Hardware Adapters
    LgComm_Process(ctx.comm);
    ctx.sensor->process();

    // 2. Update Sensor Data Cache
    if (ctx.sensor->get_data(&ctx.current_data) == LG_OK)
    {
        // Data updated
    }

    // 3. Check for Commands
    if (LgComm_Read(ctx.comm, &ctx.rx_packet) == LG_OK)
    {
        handle_command();
    }

    return LG_OK;
}

/* ============================================================================
 * Private Functions
 * ========================================================================= */
static void handle_command(void)
{
    lg_result_t res = LG_OK;
    uint8_t response_data[256];
    uint16_t response_len = 0;
    uint8_t response_cmd = 0;    // Command ID to send in response
    uint32_t response_flags = 0; // 🆕 FLAGS for cascade control

    switch (ctx.rx_packet.cmd)
    {
    case CMD_READ_RAW:
        // Payload: None
        // Response: uint16_t raw[10]
        memcpy(response_data, ctx.current_data.raw, sizeof(ctx.current_data.raw));
        response_len = sizeof(ctx.current_data.raw);
        response_cmd = CMD_READ_RAW_RESP; // 0x91
        break;

    case CMD_READ_SENSOR:
        // Payload: None (Single read mode)
        // Response: float calibrated[10]
        memcpy(response_data, ctx.current_data.calibrated, sizeof(ctx.current_data.calibrated));
        response_len = sizeof(ctx.current_data.calibrated);
        response_cmd = CMD_READ_SENSOR_RESP; // 0x90
        break;

    case CMD_READ_CASCADE:
        // 🆕 Cascade read: All sensors listen, FLAGS indicates who responds
        // FLAGS = sensor# that should respond (1-11)
        // Each sensor checks: if (FLAGS == my_address) { respond + set FLAGS=my_address+1 }

        if (ctx.rx_packet.flags == ctx.config.address)
        {
            // It's my turn to respond!
            memcpy(response_data, ctx.current_data.calibrated, sizeof(ctx.current_data.calibrated));
            response_len = sizeof(ctx.current_data.calibrated);
            response_cmd = CMD_READ_CASCADE_RESP; // 0x92

            // Set FLAGS to next sensor address (cascade trigger)
            response_flags = ctx.config.address + 1;
            if (response_flags > 11)
            {
                response_flags = 0; // End of chain (no more sensors)
            }
        }
        else
        {
            // Not my turn, ignore (don't respond)
            return; // Exit without sending anything
        }
        break;

    case CMD_GET_STATUS:
        // Payload: None
        // Response: uint16_t digital_state
        memcpy(response_data, &ctx.current_data.digital_state, sizeof(uint16_t));
        response_len = sizeof(uint16_t);
        response_cmd = CMD_GET_STATUS_RESP; // 0xB1
        break;

    case CMD_SET_OFFSET:
        // Payload: float offset[10] or specific channel?
        // Let's assume full array for now or we need a protocol definition.
        response_cmd = CMD_WRITE_CONFIG_RESP; // 0xA0
        if (ctx.rx_packet.len == sizeof(ctx.config.offset))
        {
            memcpy(ctx.config.offset, ctx.rx_packet.data, sizeof(ctx.config.offset));
            // Update sensor context?
            // Wait, Sensor Adapter holds its own offset. We need to pass it down.
            // The current ISensor doesn't have 'set_offset'.
            // Refactor Note: Add 'set_offset' to ISensor or reload config.
            // For now, we save to storage. The Sensor Adapter needs to know.
            // Re-init sensor? Too heavy.
            // Ideally, ISensor should expose 'set_calibration'.
            // I'll skip dynamic update for a moment and just save to storage.
            ctx.storage->save_config(&ctx.config);
            response_len = 0; // Ack (empty payload)
        }
        else
        {
            res = LG_INVALID_PARAM;
        }
        break;

    case CMD_SET_FILTER:
        // Payload: float fc
        response_cmd = CMD_SET_FILTER; // Echo command for ACK
        if (ctx.rx_packet.len == sizeof(float))
        {
            float fc;
            memcpy(&fc, ctx.rx_packet.data, sizeof(float));
            ctx.config.fc = fc;
            ctx.sensor->set_filter(fc);
            ctx.storage->save_config(&ctx.config);
            response_len = 0; // Ack (empty payload)
        }
        else
        {
            res = LG_INVALID_PARAM;
        }
        break;

    default:
        res = LG_INVALID_PARAM;
        break;
    }

    if (res == LG_OK)
    {
        // Send with FLAGS if cascade mode, otherwise normal send
        if (response_flags != 0)
        {
            // Cascade mode: use FLAGS field
            LgComm_SendWithFlags(ctx.comm, response_cmd, response_flags,
                                 response_data, response_len);
        }
        else
        {
            // Normal mode: send response command
            LgComm_Send(ctx.comm, response_cmd, response_data, response_len);
        }
    }
    else
    {
        // Send Error: Set bit 7 to indicate error (e.g., 0x10 → 0x90)
        uint8_t err = res;
        LgComm_Send(ctx.comm, ctx.rx_packet.cmd | 0x80, &err, 1);
    }
}
