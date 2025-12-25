#include "link_state.h"

#include <cstring>

namespace display::comm {
namespace {
std::array<PadTelemetry, NUM_PADS> padTelemetry{};
std::array<PadConfigSnapshot, NUM_PADS> padConfigs{};
SystemTelemetry systemTelemetry{};
PadTelemetry padFallback{};
PadConfigSnapshot configFallback{};
bool initialized = false;

void copyString(char* dest, size_t len, const char* src) {
    if (!dest || len == 0) {
        return;
    }
    if (!src) {
        src = "";
    }
    std::strncpy(dest, src, len - 1);
    dest[len - 1] = '\0';
}

void ensureInit() {
    if (initialized) {
        return;
    }
    LinkState::init();
}
}  // namespace

void LinkState::init() {
    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        padTelemetry[i] = PadTelemetry{
            .padId = i,
            .state = 0,
            .signal = 0,
            .baseline = 0,
            .peak = 0,
            .lastUpdateMs = 0,
            .valid = false};

        padConfigs[i] = PadConfigSnapshot{
            .padId = i,
            .threshold = 0,
            .velocityMin = 0,
            .velocityMax = 0,
            .velocityCurve = 0.0f,
            .midiNote = 0,
            .midiChannel = 0,
            .ledColorHit = 0,
            .ledColorIdle = 0,
            .ledBrightness = 0,
            .sampleName = {0},
            .name = {0},
            .enabled = false,
            .valid = false};
    }

    systemTelemetry = SystemTelemetry{
        .cpuCore0 = 0,
        .cpuCore1 = 0,
        .freeHeap = 0,
        .freePSRAM = 0,
        .temperature = 0,
        .uptime = 0,
        .lastUpdateMs = 0,
        .valid = false};

    initialized = true;
}

void LinkState::updatePadState(const PadStateMsg& msg) {
    ensureInit();
    if (msg.padId >= padTelemetry.size()) {
        return;
    }

    PadTelemetry& slot = padTelemetry[msg.padId];
    slot.padId = msg.padId;
    slot.state = msg.state;
    slot.signal = msg.currentSignal;
    slot.baseline = msg.baseline;
    slot.peak = msg.peakValue;
    slot.lastUpdateMs = millis();
    slot.valid = true;
}

void LinkState::updateSystemStatus(const SystemStatusMsg& msg) {
    ensureInit();
    systemTelemetry.cpuCore0 = msg.cpuCore0;
    systemTelemetry.cpuCore1 = msg.cpuCore1;
    systemTelemetry.freeHeap = msg.freeHeap;
    systemTelemetry.freePSRAM = msg.freePSRAM;
    systemTelemetry.temperature = msg.temperature;
    systemTelemetry.uptime = msg.uptime;
    systemTelemetry.lastUpdateMs = millis();
    systemTelemetry.valid = true;
}

void LinkState::applyPadConfig(uint8_t padId, const JsonVariantConst& source) {
    if (!source.is<JsonObjectConst>()) {
        return;
    }
    if (padId >= padConfigs.size()) {
        return;
    }

    PadConfigSnapshot& cfg = padConfigs[padId];
    cfg.padId = padId;
    if (source.containsKey("threshold")) cfg.threshold = source["threshold"];
    if (source.containsKey("velocityMin")) cfg.velocityMin = source["velocityMin"];
    if (source.containsKey("velocityMax")) cfg.velocityMax = source["velocityMax"];
    if (source.containsKey("velocityCurve")) cfg.velocityCurve = source["velocityCurve"];
    if (source.containsKey("midiNote")) cfg.midiNote = source["midiNote"];
    if (source.containsKey("midiChannel")) cfg.midiChannel = source["midiChannel"];
    if (source.containsKey("ledColorHit")) cfg.ledColorHit = source["ledColorHit"];
    if (source.containsKey("ledColorIdle")) cfg.ledColorIdle = source["ledColorIdle"];
    if (source.containsKey("ledBrightness")) cfg.ledBrightness = source["ledBrightness"];
    if (source.containsKey("sampleName")) copyString(cfg.sampleName, sizeof(cfg.sampleName), source["sampleName"]);
    if (source.containsKey("name")) copyString(cfg.name, sizeof(cfg.name), source["name"]);
    if (source.containsKey("enabled")) cfg.enabled = source["enabled"];

    cfg.valid = true;
}

void LinkState::updateConfigJSON(const char* json) {
    ensureInit();
    if (!json || json[0] == '\0') {
        return;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Display][UART] JSON parse error: %s\n", err.c_str());
        return;
    }

    if (doc.containsKey("pads")) {
        JsonArrayConst pads = doc["pads"].as<JsonArrayConst>();
        uint8_t idx = 0;
        for (JsonVariantConst pad : pads) {
            applyPadConfig(idx++, pad);
        }
    } else if (doc.containsKey("padId")) {
        uint8_t padId = doc["padId"] | 0;
        applyPadConfig(padId, doc.as<JsonVariantConst>());
    }
}

const PadTelemetry& LinkState::getPadTelemetry(uint8_t padId) {
    ensureInit();
    if (padId >= padTelemetry.size()) {
        return padFallback;
    }
    return padTelemetry[padId];
}

const SystemTelemetry& LinkState::getSystemTelemetry() {
    ensureInit();
    return systemTelemetry;
}

const PadConfigSnapshot& LinkState::getPadConfig(uint8_t padId) {
    ensureInit();
    if (padId >= padConfigs.size()) {
        return configFallback;
    }
    return padConfigs[padId];
}

}  // namespace display::comm
