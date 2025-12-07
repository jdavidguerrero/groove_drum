#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <Arduino.h>
#include "pad_config.h"

// ============================================================================
// UART PROTOCOL FOR MAIN BRAIN <-> GUI ESP COMMUNICATION
// ============================================================================
// Binary protocol with JSON payloads for efficiency and flexibility
// Baudrate: 921600 (fast enough for real-time GUI updates)
// Format: [START_BYTE][MSG_TYPE][LENGTH_MSB][LENGTH_LSB][PAYLOAD...][CRC16]

// Protocol constants
#define UART_START_BYTE 0xAA
#define UART_MAX_PAYLOAD 512
#define UART_TIMEOUT_MS 100

// Message types (Main Brain -> GUI)
enum UARTMessageType : uint8_t {
    // Events from Main Brain
    MSG_HIT_EVENT = 0x01,           // Pad hit with velocity
    MSG_PAD_STATE = 0x02,           // Current pad state (for debugging)
    MSG_SYSTEM_STATUS = 0x03,       // CPU load, memory, temp, etc.
    MSG_CONFIG_UPDATE = 0x04,       // Pad config changed
    MSG_CALIBRATION_DATA = 0x05,    // Baseline/threshold calibration info

    // Responses from Main Brain
    MSG_ACK = 0x10,                 // Command acknowledged
    MSG_NACK = 0x11,                // Command rejected (error)
    MSG_CONFIG_DUMP = 0x12,         // Full config JSON dump
    MSG_SAMPLE_LIST = 0x13,         // Available samples on SD card

    // Commands from GUI (Main Brain receives these)
    CMD_SET_THRESHOLD = 0x20,       // Update pad threshold
    CMD_SET_VELOCITY_RANGE = 0x21,  // Update velocity mapping
    CMD_SET_VELOCITY_CURVE = 0x22,  // Update velocity curve
    CMD_SET_MIDI_NOTE = 0x23,       // Change MIDI note
    CMD_SET_SAMPLE = 0x24,          // Change sample assignment
    CMD_SET_LED_COLOR = 0x25,       // Update LED colors
    CMD_SET_CROSSTALK = 0x26,       // Configure crosstalk
    CMD_SET_FULL_CONFIG = 0x27,     // Update entire pad config (JSON)

    CMD_GET_CONFIG = 0x30,          // Request config dump
    CMD_SAVE_CONFIG = 0x31,         // Save to NVS
    CMD_LOAD_CONFIG = 0x32,         // Load from NVS
    CMD_RESET_CONFIG = 0x33,        // Reset to defaults

    CMD_START_CALIBRATION = 0x40,   // Enter calibration mode
    CMD_STOP_CALIBRATION = 0x41,    // Exit calibration mode
    CMD_GET_SAMPLE_LIST = 0x42,     // List available samples

    CMD_REBOOT = 0xFF               // Reboot Main Brain
};

// ============================================================================
// MESSAGE STRUCTURES
// ============================================================================

// Packed structures for binary efficiency
#pragma pack(push, 1)

struct HitEventMsg {
    uint8_t padId;
    uint8_t velocity;       // MIDI velocity (0-127)
    uint32_t timestamp;     // Microseconds since boot
    uint16_t peakValue;     // Raw ADC peak for debugging
};

struct PadStateMsg {
    uint8_t padId;
    uint8_t state;          // TriggerState enum value
    uint16_t currentSignal;
    uint16_t baseline;
    uint16_t peakValue;
};

struct SystemStatusMsg {
    uint8_t cpuCore0;       // CPU load % (0-100)
    uint8_t cpuCore1;
    uint32_t freeHeap;      // Bytes
    uint32_t freePSRAM;     // Bytes
    int16_t temperature;    // °C * 10 (e.g., 235 = 23.5°C)
    uint32_t uptime;        // Seconds since boot
};

struct SetThresholdCmd {
    uint8_t padId;
    uint16_t threshold;
};

struct SetVelocityRangeCmd {
    uint8_t padId;
    uint16_t velocityMin;
    uint16_t velocityMax;
};

struct SetVelocityCurveCmd {
    uint8_t padId;
    float curve;
};

struct SetMidiNoteCmd {
    uint8_t padId;
    uint8_t midiNote;
    uint8_t midiChannel;
};

struct SetSampleCmd {
    uint8_t padId;
    char sampleName[32];
};

struct SetLEDColorCmd {
    uint8_t padId;
    uint32_t colorHit;      // 0xRRGGBB
    uint32_t colorIdle;
    uint8_t brightness;
};

struct SetCrosstalkCmd {
    uint8_t padId;
    uint8_t enabled;
    uint16_t window;
    float ratio;
};

struct CalibrationDataMsg {
    uint8_t padId;
    uint16_t baseline;
    uint16_t noiseFloor;    // Peak-to-peak noise
    uint16_t suggestedThreshold;
};

#pragma pack(pop)

// ============================================================================
// UART PROTOCOL HANDLER
// ============================================================================

class UARTProtocol {
public:
    // Initialize UART communication
    static void begin(HardwareSerial& serial, uint32_t baudrate = 921600);

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
