#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <Arduino.h>
#include "pad_config.h"
#include "gui_protocol.h"

// ============================================================================
// UART PROTOCOL FOR MAIN BRAIN <-> GUI ESP COMMUNICATION
// ============================================================================
class UARTProtocol {
public:
    // Initialize UART communication
    static void begin(HardwareSerial& serial, uint32_t baudrate = 921600,
                      int8_t rxPin = -1, int8_t txPin = -1);

    // Send messages to GUI
    static void sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp, uint16_t peakValue);
    static void sendPadState(uint8_t padId, uint8_t state, uint16_t signal, uint16_t baseline, uint16_t peak);
    static void sendSystemStatus();
    static void sendConfigUpdate(uint8_t padId);
    static void sendConfigDump();
    static void sendCalibrationData(uint8_t padId, uint16_t baseline, uint16_t noise, uint16_t suggested);
    static void sendAck(uint8_t cmdType);
    static void sendNack(uint8_t cmdType, const char* error);

    // Process incoming messages from GUI
    static void processIncoming();

    // Statistics
    static uint32_t getTxCount() { return txCount; }
    static uint32_t getRxCount() { return rxCount; }
    static uint32_t getErrorCount() { return errorCount; }

private:
    static HardwareSerial* uart;
    static uint32_t txCount;
    static uint32_t rxCount;
    static uint32_t errorCount;

    // Low-level protocol
    static void sendMessage(uint8_t msgType, const void* payload, uint16_t length);
    static bool receiveMessage(uint8_t& msgType, uint8_t* payload, uint16_t& length);
    static uint16_t calculateCRC16(const uint8_t* data, uint16_t length, uint16_t crc = 0xFFFF);

    // Command handlers
    static void handleSetThreshold(const SetThresholdCmd& cmd);
    static void handleSetVelocityRange(const SetVelocityRangeCmd& cmd);
    static void handleSetVelocityCurve(const SetVelocityCurveCmd& cmd);
    static void handleSetMidiNote(const SetMidiNoteCmd& cmd);
    static void handleSetSample(const SetSampleCmd& cmd);
    static void handleSetLEDColor(const SetLEDColorCmd& cmd);
    static void handleSetCrosstalk(const SetCrosstalkCmd& cmd);
    static void handleSetFullConfig(const char* json);
    static void handleGetConfig();
    static void handleSaveConfig();
    static void handleLoadConfig();
    static void handleResetConfig(uint8_t padId);
};

#endif // UART_PROTOCOL_H
