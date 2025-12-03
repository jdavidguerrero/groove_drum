/**
 * @file protocol.cpp
 * @brief Implementation of UART communication protocol
 * @version 1.0
 * @date 2025-12-02
 */

#include "protocol.h"

// ============================================================
// CRC-8 CALCULATION
// ============================================================

uint8_t protocolCalculateCRC8(const uint8_t* data, uint8_t length) {
    uint8_t crc = 0x00;

    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ PROTOCOL_CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// ============================================================
// FRAME ENCODING/DECODING
// ============================================================

uint8_t protocolEncodeFrame(const ProtocolFrame& frame, uint8_t* buffer) {
    uint8_t idx = 0;

    // SYNC byte
    buffer[idx++] = frame.sync;

    // LENGTH byte
    buffer[idx++] = frame.length;

    // COMMAND byte
    buffer[idx++] = frame.command;

    // PAYLOAD
    for (uint8_t i = 0; i < frame.length; i++) {
        buffer[idx++] = frame.payload[i];
    }

    // Calculate CRC over LEN + CMD + PAYLOAD
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    uint8_t crc = protocolCalculateCRC8(crcData, 2 + frame.length);
    buffer[idx++] = crc;

    // END byte
    buffer[idx++] = frame.end;

    return idx;
}

bool protocolDecodeFrame(const uint8_t* buffer, uint8_t bufferLen, ProtocolFrame& frame) {
    // Minimum frame size: SYNC + LEN + CMD + CRC + END = 5 bytes
    if (bufferLen < 5) {
        return false;
    }

    uint8_t idx = 0;

    // Check SYNC byte
    if (buffer[idx] != PROTOCOL_SYNC_BYTE) {
        return false;
    }
    frame.sync = buffer[idx++];

    // Read LENGTH
    frame.length = buffer[idx++];
    if (frame.length > PROTOCOL_MAX_PAYLOAD) {
        return false;
    }

    // Check buffer has enough data
    if (bufferLen < (5 + frame.length)) {
        return false;
    }

    // Read COMMAND
    frame.command = buffer[idx++];

    // Read PAYLOAD
    for (uint8_t i = 0; i < frame.length; i++) {
        frame.payload[i] = buffer[idx++];
    }

    // Read CRC
    frame.crc = buffer[idx++];

    // Check END byte
    if (buffer[idx] != PROTOCOL_END_BYTE) {
        return false;
    }
    frame.end = buffer[idx++];

    // Validate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    uint8_t expectedCrc = protocolCalculateCRC8(crcData, 2 + frame.length);

    if (frame.crc != expectedCrc) {
        return false;
    }

    return true;
}

// ============================================================
// FRAME VALIDATION
// ============================================================

bool protocolValidateFrame(const ProtocolFrame& frame) {
    // Check sync and end bytes
    if (frame.sync != PROTOCOL_SYNC_BYTE || frame.end != PROTOCOL_END_BYTE) {
        return false;
    }

    // Check length
    if (frame.length > PROTOCOL_MAX_PAYLOAD) {
        return false;
    }

    // Validate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    uint8_t expectedCrc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return (frame.crc == expectedCrc);
}

// ============================================================
// FRAME CREATION HELPERS
// ============================================================

ProtocolFrame protocolCreatePadHit(uint8_t padId, uint8_t velocity, uint8_t flags) {
    ProtocolFrame frame;
    frame.command = CMD_PAD_HIT;
    frame.length = 3;

    PayloadPadHit payload;
    payload.pad_id = padId;
    payload.velocity = velocity;
    payload.flags = flags;

    memcpy(frame.payload, &payload, sizeof(PayloadPadHit));

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

ProtocolFrame protocolCreateButtonEvent(uint8_t buttonId, uint8_t state) {
    ProtocolFrame frame;
    frame.command = CMD_BUTTON_EVENT;
    frame.length = 2;
    frame.payload[0] = buttonId;
    frame.payload[1] = state;

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

ProtocolFrame protocolCreateEncoderRotate(uint8_t encoderId, int8_t delta, uint8_t flags) {
    ProtocolFrame frame;
    frame.command = CMD_ENCODER_ROTATE;
    frame.length = 3;

    PayloadEncoderRotate payload;
    payload.encoder_id = encoderId;
    payload.delta = delta;
    payload.flags = flags;

    memcpy(frame.payload, &payload, sizeof(PayloadEncoderRotate));

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

ProtocolFrame protocolCreateHeartbeat(uint32_t uptimeMs) {
    ProtocolFrame frame;
    frame.command = CMD_HEARTBEAT;
    frame.length = 4;

    PayloadHeartbeat payload;
    payload.uptime_ms = uptimeMs;

    memcpy(frame.payload, &payload, sizeof(PayloadHeartbeat));

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

ProtocolFrame protocolCreateError(uint8_t errorCode, uint8_t context) {
    ProtocolFrame frame;
    frame.command = CMD_ERROR;
    frame.length = 2;

    PayloadError payload;
    payload.error_code = errorCode;
    payload.context = context;

    memcpy(frame.payload, &payload, sizeof(PayloadError));

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

ProtocolFrame protocolCreateAck(uint8_t ackedCommand) {
    ProtocolFrame frame;
    frame.command = CMD_ACK;
    frame.length = 1;
    frame.payload[0] = ackedCommand;

    // Calculate CRC
    uint8_t crcData[1 + 1 + PROTOCOL_MAX_PAYLOAD];
    crcData[0] = frame.length;
    crcData[1] = frame.command;
    memcpy(&crcData[2], frame.payload, frame.length);
    frame.crc = protocolCalculateCRC8(crcData, 2 + frame.length);

    return frame;
}

// ============================================================
// DEBUG HELPERS
// ============================================================

const char* protocolGetCommandName(uint8_t command) {
    switch (command) {
        // MCU#1 → MCU#2
        case CMD_PAD_HIT:           return "PAD_HIT";
        case CMD_PAD_RELEASE:       return "PAD_RELEASE";
        case CMD_ENCODER_ROTATE:    return "ENCODER_ROTATE";
        case CMD_ENCODER_PUSH:      return "ENCODER_PUSH";
        case CMD_BUTTON_EVENT:      return "BUTTON_EVENT";
        case CMD_KIT_INFO:          return "KIT_INFO";
        case CMD_PAD_CONFIG:        return "PAD_CONFIG";
        case CMD_GLOBAL_STATE:      return "GLOBAL_STATE";
        case CMD_MIDI_ACTIVITY:     return "MIDI_ACTIVITY";
        case CMD_SYNC_REQUEST:      return "SYNC_REQUEST";
        case CMD_ACK:               return "ACK";
        case CMD_ERROR:             return "ERROR";
        case CMD_HEARTBEAT:         return "HEARTBEAT";

        // MCU#2 → MCU#1
        case CMD_PARAM_CHANGE:      return "PARAM_CHANGE";
        case CMD_KIT_SELECT:        return "KIT_SELECT";
        case CMD_VIEW_CHANGED:      return "VIEW_CHANGED";
        case CMD_REQUEST_KIT_INFO:  return "REQUEST_KIT_INFO";
        case CMD_REQUEST_PAD_CONFIG:return "REQUEST_PAD_CONFIG";
        case CMD_ACK_DISPLAY:       return "ACK_DISPLAY";
    }

    // Aliases that share values with CMD_ERROR/CMD_HEARTBEAT
    if (command == CMD_ERROR_DISPLAY) {
        return "ERROR";
    }
    if (command == CMD_HEARTBEAT_DISPLAY) {
        return "HEARTBEAT";
    }

    return "UNKNOWN";
}
