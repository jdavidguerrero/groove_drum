#include "uart_protocol.h"
#include "pad_config.h"
#include <ArduinoJson.h>
#include <esp_system.h>

// Static member initialization
HardwareSerial* UARTProtocol::uart = nullptr;
uint32_t UARTProtocol::txCount = 0;
uint32_t UARTProtocol::rxCount = 0;
uint32_t UARTProtocol::errorCount = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void UARTProtocol::begin(HardwareSerial& serial, uint32_t baudrate, int8_t rxPin, int8_t txPin) {
    uart = &serial;
    uart->end();
    uart->setRxBufferSize(4096);  // Larger buffer for config messages
    if (rxPin >= 0 || txPin >= 0) {
        uart->begin(baudrate, SERIAL_8N1, rxPin, txPin);
    } else {
        uart->begin(baudrate);
    }
    Serial.printf("[UART] Protocol initialized at %d baud\n", baudrate);
}

// ============================================================================
// SEND MESSAGES TO GUI
// ============================================================================

void UARTProtocol::sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp, uint16_t peakValue) {
    HitEventMsg msg = {
        .padId = padId,
        .velocity = velocity,
        .timestamp = timestamp,
        .peakValue = peakValue
    };
    sendMessage(MSG_HIT_EVENT, &msg, sizeof(msg));
}

void UARTProtocol::sendPadState(uint8_t padId, uint8_t state, uint16_t signal, uint16_t baseline, uint16_t peak) {
    PadStateMsg msg = {
        .padId = padId,
        .state = state,
        .currentSignal = signal,
        .baseline = baseline,
        .peakValue = peak
    };
    sendMessage(MSG_PAD_STATE, &msg, sizeof(msg));
}

void UARTProtocol::sendSystemStatus() {
    SystemStatusMsg msg = {
        .cpuCore0 = 0,  // TODO: Implement CPU monitoring
        .cpuCore1 = 0,
        .freeHeap = ESP.getFreeHeap(),
        .freePSRAM = ESP.getFreePsram(),
        .temperature = (int16_t)(temperatureRead() * 10),
        .uptime = (uint32_t)(millis() / 1000)
    };
    sendMessage(MSG_SYSTEM_STATUS, &msg, sizeof(msg));
}

void UARTProtocol::sendConfigUpdate(uint8_t padId) {
    // Send individual pad config as JSON
    DynamicJsonDocument doc(256);
    PadConfig& cfg = PadConfigManager::getConfig(padId);

    doc["padId"] = padId;
    doc["threshold"] = cfg.threshold;
    doc["velocityMin"] = cfg.velocityMin;
    doc["velocityMax"] = cfg.velocityMax;
    doc["velocityCurve"] = cfg.velocityCurve;
    doc["midiNote"] = cfg.midiNote;
    doc["sampleName"] = cfg.sampleName;
    doc["ledColorHit"] = cfg.ledColorHit;
    doc["ledColorIdle"] = cfg.ledColorIdle;

    String json;
    serializeJson(doc, json);
    sendMessage(MSG_CONFIG_UPDATE, json.c_str(), json.length() + 1);
}

void UARTProtocol::sendConfigDump() {
    String json = PadConfigManager::exportJSON();
    sendMessage(MSG_CONFIG_DUMP, json.c_str(), json.length() + 1);
}

void UARTProtocol::sendCalibrationData(uint8_t padId, uint16_t baseline, uint16_t noise, uint16_t suggested) {
    CalibrationDataMsg msg = {
        .padId = padId,
        .baseline = baseline,
        .noiseFloor = noise,
        .suggestedThreshold = suggested
    };
    sendMessage(MSG_CALIBRATION_DATA, &msg, sizeof(msg));
}

void UARTProtocol::sendAck(uint8_t cmdType) {
    sendMessage(MSG_ACK, &cmdType, 1);
}

void UARTProtocol::sendNack(uint8_t cmdType, const char* error) {
    uint8_t buffer[64];
    buffer[0] = cmdType;
    strncpy((char*)&buffer[1], error, 62);
    buffer[63] = '\0';
    sendMessage(MSG_NACK, buffer, strlen((char*)buffer) + 1);
}

void UARTProtocol::sendRawCommand(uint8_t cmdType, const uint8_t* data, uint16_t length) {
    sendMessage(cmdType, data, length);
}

void UARTProtocol::sendMenuState(const MenuStateMsg& msg) {
    sendMessage(MSG_MENU_STATE, &msg, sizeof(msg));
}

void UARTProtocol::sendSampleList(const SampleListMsg& msg) {
    sendMessage(MSG_MENU_SAMPLES, &msg, sizeof(msg));
}

// ============================================================================
// PROCESS INCOMING MESSAGES FROM GUI
// ============================================================================

void UARTProtocol::processIncoming() {
    if (!uart || uart->available() < 5) return;  // Need at least header

    uint8_t msgType;
    uint8_t payload[UART_MAX_PAYLOAD];
    uint16_t length;

    if (receiveMessage(msgType, payload, length)) {
        rxCount++;

        // Dispatch to handlers
        switch (msgType) {
            case CMD_SET_THRESHOLD:
                if (length == sizeof(SetThresholdCmd)) {
                    handleSetThreshold(*(SetThresholdCmd*)payload);
                }
                break;

            case CMD_SET_VELOCITY_RANGE:
                if (length == sizeof(SetVelocityRangeCmd)) {
                    handleSetVelocityRange(*(SetVelocityRangeCmd*)payload);
                }
                break;

            case CMD_SET_VELOCITY_CURVE:
                if (length == sizeof(SetVelocityCurveCmd)) {
                    handleSetVelocityCurve(*(SetVelocityCurveCmd*)payload);
                }
                break;

            case CMD_SET_MIDI_NOTE:
                if (length == sizeof(SetMidiNoteCmd)) {
                    handleSetMidiNote(*(SetMidiNoteCmd*)payload);
                }
                break;

            case CMD_SET_SAMPLE:
                if (length == sizeof(SetSampleCmd)) {
                    handleSetSample(*(SetSampleCmd*)payload);
                }
                break;

            case CMD_SET_LED_COLOR:
                if (length == sizeof(SetLEDColorCmd)) {
                    handleSetLEDColor(*(SetLEDColorCmd*)payload);
                }
                break;

            case CMD_SET_CROSSTALK:
                if (length == sizeof(SetCrosstalkCmd)) {
                    handleSetCrosstalk(*(SetCrosstalkCmd*)payload);
                }
                break;

            case CMD_SET_FULL_CONFIG:
                handleSetFullConfig((char*)payload);
                break;

            case CMD_GET_CONFIG:
                handleGetConfig();
                break;

            case CMD_SAVE_CONFIG:
                handleSaveConfig();
                break;

            case CMD_LOAD_CONFIG:
                handleLoadConfig();
                break;

            case CMD_RESET_CONFIG:
                if (length >= 1) {
                    handleResetConfig(payload[0]);
                }
                break;

            case CMD_REBOOT:
                sendAck(CMD_REBOOT);
                delay(100);
                ESP.restart();
                break;

            default:
                sendNack(msgType, "Unknown command");
                errorCount++;
                break;
        }
    }
}

// ============================================================================
// LOW-LEVEL PROTOCOL
// ============================================================================

void UARTProtocol::sendMessage(uint8_t msgType, const void* payload, uint16_t length) {
    if (!uart || length > UART_MAX_PAYLOAD) return;

    uint8_t header[4];
    header[0] = UART_START_BYTE;
    header[1] = msgType;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;

    // Calculate CRC over header + payload
    uint16_t crc = calculateCRC16(header, 4);
    crc = calculateCRC16((const uint8_t*)payload, length, crc);

    // Send: [HEADER][PAYLOAD][CRC]
    uart->write(header, 4);
    uart->write((const uint8_t*)payload, length);
    uart->write((crc >> 8) & 0xFF);
    uart->write(crc & 0xFF);

    txCount++;
}

bool UARTProtocol::receiveMessage(uint8_t& msgType, uint8_t* payload, uint16_t& length) {
    if (!uart) return false;

    // Wait for start byte
    uint32_t startTime = millis();
    while (uart->available() > 0 && uart->peek() != UART_START_BYTE) {
        uart->read();  // Discard garbage
        if (millis() - startTime > UART_TIMEOUT_MS) return false;
    }

    if (uart->available() < 4) return false;

    // Read header
    uint8_t header[4];
    uart->readBytes(header, 4);

    if (header[0] != UART_START_BYTE) {
        errorCount++;
        return false;
    }

    msgType = header[1];
    length = (header[2] << 8) | header[3];

    if (length > UART_MAX_PAYLOAD) {
        errorCount++;
        return false;
    }

    // Wait for payload + CRC
    startTime = millis();
    while (uart->available() < length + 2) {
        if (millis() - startTime > UART_TIMEOUT_MS) {
            errorCount++;
            return false;
        }
        delay(1);
    }

    // Read payload
    uart->readBytes(payload, length);

    // Read CRC
    uint16_t receivedCRC = (uart->read() << 8) | uart->read();

    // Verify CRC
    uint16_t calculatedCRC = calculateCRC16(header, 4);
    calculatedCRC = calculateCRC16(payload, length, calculatedCRC);

    if (receivedCRC != calculatedCRC) {
        errorCount++;
        return false;
    }

    return true;
}

uint16_t UARTProtocol::calculateCRC16(const uint8_t* data, uint16_t length, uint16_t crc) {
    // CRC-16-CCITT (polynomial 0x1021)
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

void UARTProtocol::handleSetThreshold(const SetThresholdCmd& cmd) {
    PadConfigManager::setThreshold(cmd.padId, cmd.threshold);
    sendAck(CMD_SET_THRESHOLD);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] Threshold updated: Pad %d = %d\n", cmd.padId, cmd.threshold);
}

void UARTProtocol::handleSetVelocityRange(const SetVelocityRangeCmd& cmd) {
    PadConfigManager::setVelocityRange(cmd.padId, cmd.velocityMin, cmd.velocityMax);
    sendAck(CMD_SET_VELOCITY_RANGE);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] Velocity range updated: Pad %d = [%d-%d]\n",
                  cmd.padId, cmd.velocityMin, cmd.velocityMax);
}

void UARTProtocol::handleSetVelocityCurve(const SetVelocityCurveCmd& cmd) {
    PadConfigManager::setVelocityCurve(cmd.padId, cmd.curve);
    sendAck(CMD_SET_VELOCITY_CURVE);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] Velocity curve updated: Pad %d = %.2f\n", cmd.padId, cmd.curve);
}

void UARTProtocol::handleSetMidiNote(const SetMidiNoteCmd& cmd) {
    PadConfigManager::setMidiNote(cmd.padId, cmd.midiNote);
    sendAck(CMD_SET_MIDI_NOTE);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] MIDI note updated: Pad %d = %d\n", cmd.padId, cmd.midiNote);
}

void UARTProtocol::handleSetSample(const SetSampleCmd& cmd) {
    PadConfigManager::setSample(cmd.padId, cmd.sampleName);
    sendAck(CMD_SET_SAMPLE);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] Sample updated: Pad %d = %s\n", cmd.padId, cmd.sampleName);
}

void UARTProtocol::handleSetLEDColor(const SetLEDColorCmd& cmd) {
    PadConfigManager::setLEDColor(cmd.padId, cmd.colorHit, cmd.colorIdle);
    sendAck(CMD_SET_LED_COLOR);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] LED colors updated: Pad %d\n", cmd.padId);
}

void UARTProtocol::handleSetCrosstalk(const SetCrosstalkCmd& cmd) {
    PadConfigManager::setCrosstalk(cmd.padId, cmd.enabled, cmd.window, cmd.ratio);
    sendAck(CMD_SET_CROSSTALK);
    sendConfigUpdate(cmd.padId);
    Serial.printf("[UART] Crosstalk updated: Pad %d\n", cmd.padId);
}

void UARTProtocol::handleSetFullConfig(const char* json) {
    if (PadConfigManager::importJSON(json)) {
        sendAck(CMD_SET_FULL_CONFIG);
        sendConfigDump();
    } else {
        sendNack(CMD_SET_FULL_CONFIG, "Invalid JSON");
    }
}

void UARTProtocol::handleGetConfig() {
    sendConfigDump();
}

void UARTProtocol::handleSaveConfig() {
    if (PadConfigManager::saveToNVS()) {
        sendAck(CMD_SAVE_CONFIG);
    } else {
        sendNack(CMD_SAVE_CONFIG, "NVS write failed");
    }
}

void UARTProtocol::handleLoadConfig() {
    if (PadConfigManager::loadFromNVS()) {
        sendAck(CMD_LOAD_CONFIG);
        sendConfigDump();
    } else {
        sendNack(CMD_LOAD_CONFIG, "NVS read failed");
    }
}

void UARTProtocol::handleResetConfig(uint8_t padId) {
    if (padId == 0xFF) {
        PadConfigManager::resetAllToDefaults();
    } else {
        PadConfigManager::resetToDefaults(padId);
    }
    sendAck(CMD_RESET_CONFIG);
    sendConfigDump();
}
