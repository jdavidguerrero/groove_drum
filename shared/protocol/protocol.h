/**
 * @file protocol.h
 * @brief UART communication protocol between MCU #1 and MCU #2
 * @version 1.0
 * @date 2025-12-02
 *
 * Frame structure: SYNC + LEN + CMD + PAYLOAD + CRC + END
 * - SYNC: 0xAA (start of frame)
 * - LEN: Payload length (0-16 bytes)
 * - CMD: Command byte (see enums below)
 * - PAYLOAD: Command-specific data (0-16 bytes)
 * - CRC: CRC-8 checksum (polynomial 0x07)
 * - END: 0x55 (end of frame)
 */

#pragma once

#include <Arduino.h>

// ============================================================
// PROTOCOL CONSTANTS
// ============================================================

#define PROTOCOL_SYNC_BYTE    0xAA
#define PROTOCOL_END_BYTE     0x55
#define PROTOCOL_MAX_PAYLOAD  16
#define PROTOCOL_CRC_POLY     0x07  // CRC-8 polynomial

// Frame structure sizes
#define FRAME_SIZE_SYNC     1
#define FRAME_SIZE_LEN      1
#define FRAME_SIZE_CMD      1
#define FRAME_SIZE_CRC      1
#define FRAME_SIZE_END      1
#define FRAME_SIZE_OVERHEAD (FRAME_SIZE_SYNC + FRAME_SIZE_LEN + FRAME_SIZE_CMD + FRAME_SIZE_CRC + FRAME_SIZE_END)
#define FRAME_SIZE_MAX      (FRAME_SIZE_OVERHEAD + PROTOCOL_MAX_PAYLOAD)

// ============================================================
// COMMAND DEFINITIONS - MCU#1 → MCU#2
// ============================================================

enum CommandFromMain : uint8_t {
    // Input Events (0x01-0x0F)
    CMD_PAD_HIT         = 0x01,  // Payload: {pad_id, velocity, flags}
    CMD_PAD_RELEASE     = 0x02,  // Payload: {pad_id}
    CMD_ENCODER_ROTATE  = 0x03,  // Payload: {encoder_id, delta(signed), flags}
    CMD_ENCODER_PUSH    = 0x04,  // Payload: {encoder_id, state}
    CMD_BUTTON_EVENT    = 0x05,  // Payload: {button_id, state}

    // Configuration (0x10-0x1F)
    CMD_KIT_INFO        = 0x10,  // Payload: {kit_number, kit_name[10], flags}
    CMD_PAD_CONFIG      = 0x11,  // Payload: {pad_id, note, channel, vol, pan, pitch, decay, color}
    CMD_GLOBAL_STATE    = 0x12,  // Payload: {bpm(16), master_vol, click, usb_mode, sync}
    CMD_MIDI_ACTIVITY   = 0x13,  // Payload: {flags} (visual MIDI indicator)

    // System (0x20-0x2F)
    CMD_SYNC_REQUEST    = 0x20,  // Payload: none (request full state sync)
    CMD_ACK             = 0x21,  // Payload: {acked_cmd}

    // Error/Status (0xF0-0xFF)
    CMD_ERROR           = 0xFE,  // Payload: {error_code, context}
    CMD_HEARTBEAT       = 0xFF   // Payload: {uptime_ms(32)}
};

// ============================================================
// COMMAND DEFINITIONS - MCU#2 → MCU#1
// ============================================================

enum CommandFromDisplay : uint8_t {
    // UI Events (0x81-0x8F)
    CMD_PARAM_CHANGE    = 0x81,  // Payload: {target, target_id, param_id, value}
    CMD_KIT_SELECT      = 0x82,  // Payload: {kit_number}
    CMD_VIEW_CHANGED    = 0x83,  // Payload: {view_id}

    // Requests (0x90-0x9F)
    CMD_REQUEST_KIT_INFO    = 0x90,  // Payload: {kit_number}
    CMD_REQUEST_PAD_CONFIG  = 0x91,  // Payload: {pad_id}

    // System (0xA0-0xAF)
    CMD_ACK_DISPLAY         = 0xA1,  // Payload: {acked_cmd}

    // Error/Status (0xF0-0xFF)
    CMD_ERROR_DISPLAY       = 0xFE,  // Payload: {error_code, context}
    CMD_HEARTBEAT_DISPLAY   = 0xFF   // Payload: {uptime_ms(32)}
};

// ============================================================
// ERROR CODES
// ============================================================

enum ErrorCode : uint8_t {
    ERR_NONE = 0,
    ERR_CRC = 1,              // CRC checksum mismatch
    ERR_UNKNOWN_CMD = 2,      // Unknown command byte
    ERR_INVALID_LEN = 3,      // Invalid payload length
    ERR_TIMEOUT_COMM = 4,     // Communication timeout (renamed to avoid lwIP ERR_TIMEOUT)
    ERR_OVERFLOW = 5,         // Buffer overflow
    ERR_INVALID_PARAM = 6,    // Invalid parameter value
    ERR_HARDWARE = 7          // Hardware error
};

// ============================================================
// VIEW IDs (UI Screens)
// ============================================================

enum ViewID : uint8_t {
    VIEW_PERFORMANCE = 0,
    VIEW_PAD_EDIT = 1,
    VIEW_MIXER = 2,
    VIEW_SETTINGS = 3
};

// ============================================================
// PARAMETER IDs
// ============================================================

// Pad Parameters
enum PadParam : uint8_t {
    PARAM_VOLUME = 0,
    PARAM_PAN = 1,
    PARAM_PITCH = 2,
    PARAM_DECAY = 3,
    PARAM_NOTE = 4,
    PARAM_CHANNEL = 5,
    PARAM_COLOR = 6
};

// Global Parameters
enum GlobalParam : uint8_t {
    PARAM_MASTER_VOL = 0x10,
    PARAM_BPM = 0x11,
    PARAM_CLICK_EN = 0x12,
    PARAM_USB_MODE = 0x13
};

// ============================================================
// FRAME STRUCTURE
// ============================================================

/**
 * @brief Protocol frame structure
 */
struct ProtocolFrame {
    uint8_t sync;                           // 0xAA
    uint8_t length;                         // Payload length (0-16)
    uint8_t command;                        // Command byte
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];  // Payload data
    uint8_t crc;                            // CRC-8 checksum
    uint8_t end;                            // 0x55

    ProtocolFrame() : sync(PROTOCOL_SYNC_BYTE), length(0), command(0), crc(0), end(PROTOCOL_END_BYTE) {
        memset(payload, 0, PROTOCOL_MAX_PAYLOAD);
    }
};

// ============================================================
// PAYLOAD STRUCTURES
// ============================================================

/**
 * @brief Pad hit event payload
 */
struct PayloadPadHit {
    uint8_t pad_id;
    uint8_t velocity;
    uint8_t flags;  // Reserved for future use
} __attribute__((packed));

/**
 * @brief Encoder rotation event payload
 */
struct PayloadEncoderRotate {
    uint8_t encoder_id;
    int8_t delta;    // Signed rotation delta
    uint8_t flags;   // Reserved for future use
} __attribute__((packed));

/**
 * @brief Global state payload
 */
struct PayloadGlobalState {
    uint16_t bpm;
    uint8_t master_vol;
    uint8_t click_enabled;
    uint8_t usb_mode;
    uint8_t sync_flags;
} __attribute__((packed));

/**
 * @brief Heartbeat payload
 */
struct PayloadHeartbeat {
    uint32_t uptime_ms;
} __attribute__((packed));

/**
 * @brief Error payload
 */
struct PayloadError {
    uint8_t error_code;
    uint8_t context;  // Additional context (command that caused error, etc.)
} __attribute__((packed));

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

/**
 * @brief Calculate CRC-8 checksum
 * @param data Pointer to data buffer
 * @param length Length of data
 * @return CRC-8 checksum
 */
uint8_t protocolCalculateCRC8(const uint8_t* data, uint8_t length);

/**
 * @brief Encode a frame for transmission
 * @param frame Frame structure to encode
 * @param buffer Output buffer (must be at least FRAME_SIZE_MAX bytes)
 * @return Number of bytes written to buffer
 */
uint8_t protocolEncodeFrame(const ProtocolFrame& frame, uint8_t* buffer);

/**
 * @brief Decode a received frame
 * @param buffer Input buffer containing raw bytes
 * @param bufferLen Length of input buffer
 * @param frame Output frame structure
 * @return true if frame decoded successfully, false otherwise
 */
bool protocolDecodeFrame(const uint8_t* buffer, uint8_t bufferLen, ProtocolFrame& frame);

/**
 * @brief Create a pad hit event frame
 * @param padId Pad ID (0-3)
 * @param velocity MIDI velocity (1-127)
 * @param flags Additional flags
 * @return Encoded frame
 */
ProtocolFrame protocolCreatePadHit(uint8_t padId, uint8_t velocity, uint8_t flags = 0);

/**
 * @brief Create a button event frame
 * @param buttonId Button ID
 * @param state Button state
 * @return Encoded frame
 */
ProtocolFrame protocolCreateButtonEvent(uint8_t buttonId, uint8_t state);

/**
 * @brief Create an encoder rotation event frame
 * @param encoderId Encoder ID (0-1)
 * @param delta Rotation delta (signed)
 * @param flags Additional flags
 * @return Encoded frame
 */
ProtocolFrame protocolCreateEncoderRotate(uint8_t encoderId, int8_t delta, uint8_t flags = 0);

/**
 * @brief Create a heartbeat frame
 * @param uptimeMs System uptime in milliseconds
 * @return Encoded frame
 */
ProtocolFrame protocolCreateHeartbeat(uint32_t uptimeMs);

/**
 * @brief Create an error frame
 * @param errorCode Error code
 * @param context Additional context
 * @return Encoded frame
 */
ProtocolFrame protocolCreateError(uint8_t errorCode, uint8_t context);

/**
 * @brief Create an ACK frame
 * @param ackedCommand Command being acknowledged
 * @return Encoded frame
 */
ProtocolFrame protocolCreateAck(uint8_t ackedCommand);

/**
 * @brief Validate a frame (check CRC and structure)
 * @param frame Frame to validate
 * @return true if valid, false otherwise
 */
bool protocolValidateFrame(const ProtocolFrame& frame);

/**
 * @brief Get human-readable command name for debugging
 * @param command Command byte
 * @return Pointer to command name string
 */
const char* protocolGetCommandName(uint8_t command);
