#pragma once

#include <Arduino.h>
#include <array>
#include <ArduinoJson.h>
#include <gui_protocol.h>
#include <edrum_config.h>

namespace display::comm {

struct PadTelemetry {
    uint8_t padId;
    uint8_t state;
    uint16_t signal;
    uint16_t baseline;
    uint16_t peak;
    uint32_t lastUpdateMs;
    bool valid;
};

struct SystemTelemetry {
    uint8_t cpuCore0;
    uint8_t cpuCore1;
    uint32_t freeHeap;
    uint32_t freePSRAM;
    int16_t temperature;
    uint32_t uptime;
    uint32_t lastUpdateMs;
    bool valid;
};

struct PadConfigSnapshot {
    uint8_t padId;
    uint16_t threshold;
    uint16_t velocityMin;
    uint16_t velocityMax;
    float velocityCurve;
    uint8_t midiNote;
    uint8_t midiChannel;
    uint32_t ledColorHit;
    uint32_t ledColorIdle;
    uint8_t ledBrightness;
    char sampleName[32];
    char name[16];
    bool enabled;
    bool valid;
};

class LinkState {
public:
    static void init();
    static void updatePadState(const PadStateMsg& msg);
    static void updateSystemStatus(const SystemStatusMsg& msg);
    static void updateConfigJSON(const char* json);

    static const PadTelemetry& getPadTelemetry(uint8_t padId);
    static const SystemTelemetry& getSystemTelemetry();
    static const PadConfigSnapshot& getPadConfig(uint8_t padId);

private:
    static void applyPadConfig(uint8_t padId, const JsonVariantConst& source);
};

}  // namespace display::comm
