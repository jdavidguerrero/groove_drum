#ifndef GUI_PROTOCOL_H
#define GUI_PROTOCOL_H

#include <Arduino.h>

// Basic framing constants shared between MCUs
#define UART_START_BYTE 0xAA
#define UART_MAX_PAYLOAD 512
#define UART_TIMEOUT_MS 100

// Message types (bidirectional)
enum UARTMessageType : uint8_t {
    // Events from Main Brain
    MSG_HIT_EVENT = 0x01,
    MSG_PAD_STATE = 0x02,
    MSG_SYSTEM_STATUS = 0x03,
    MSG_CONFIG_UPDATE = 0x04,
    MSG_CALIBRATION_DATA = 0x05,

    // Responses from Main Brain
    MSG_ACK = 0x10,
    MSG_NACK = 0x11,
    MSG_CONFIG_DUMP = 0x12,
    MSG_SAMPLE_LIST = 0x13,

    // Commands from GUI
    CMD_SET_THRESHOLD = 0x20,
    CMD_SET_VELOCITY_RANGE = 0x21,
    CMD_SET_VELOCITY_CURVE = 0x22,
    CMD_SET_MIDI_NOTE = 0x23,
    CMD_SET_SAMPLE = 0x24,
    CMD_SET_LED_COLOR = 0x25,
    CMD_SET_CROSSTALK = 0x26,
    CMD_SET_FULL_CONFIG = 0x27,

    CMD_GET_CONFIG = 0x30,
    CMD_SAVE_CONFIG = 0x31,
    CMD_LOAD_CONFIG = 0x32,
    CMD_RESET_CONFIG = 0x33,

    CMD_START_CALIBRATION = 0x40,
    CMD_STOP_CALIBRATION = 0x41,
    CMD_GET_SAMPLE_LIST = 0x42,

    CMD_REBOOT = 0xFF
};

#pragma pack(push, 1)

struct HitEventMsg {
    uint8_t padId;
    uint8_t velocity;
    uint32_t timestamp;
    uint16_t peakValue;
};

struct PadStateMsg {
    uint8_t padId;
    uint8_t state;
    uint16_t currentSignal;
    uint16_t baseline;
    uint16_t peakValue;
};

struct SystemStatusMsg {
    uint8_t cpuCore0;
    uint8_t cpuCore1;
    uint32_t freeHeap;
    uint32_t freePSRAM;
    int16_t temperature;
    uint32_t uptime;
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
    uint32_t colorHit;
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
    uint16_t noiseFloor;
    uint16_t suggestedThreshold;
};

#pragma pack(pop)

#endif // GUI_PROTOCOL_H
