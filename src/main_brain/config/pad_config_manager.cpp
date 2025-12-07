#include "pad_config.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// Static member initialization
PadConfig PadConfigManager::configs[8];

// ============================================================================
// INITIALIZATION
// ============================================================================

void PadConfigManager::init() {
    // Load from NVS, or use defaults if not found
    if (!loadFromNVS()) {
        Serial.println("[CONFIG] No saved config found, using defaults");
        resetAllToDefaults();
    } else {
        Serial.println("[CONFIG] Loaded configuration from NVS");
    }
}

// ============================================================================
// NVS STORAGE (Non-Volatile Storage)
// ============================================================================

bool PadConfigManager::loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin("edrum", true)) {  // true = read-only
        return false;
    }

    bool success = true;
    for (uint8_t i = 0; i < 4; i++) {  // Load 4 pads
        char key[16];
        snprintf(key, sizeof(key), "pad%d", i);

        size_t len = prefs.getBytesLength(key);
        if (len == sizeof(PadConfig)) {
            prefs.getBytes(key, &configs[i], sizeof(PadConfig));
        } else {
            success = false;
            break;
        }
    }

    prefs.end();
    return success;
}

bool PadConfigManager::saveToNVS() {
    Preferences prefs;
    if (!prefs.begin("edrum", false)) {  // false = read-write
        return false;
    }

    for (uint8_t i = 0; i < 4; i++) {
        char key[16];
        snprintf(key, sizeof(key), "pad%d", i);
        prefs.putBytes(key, &configs[i], sizeof(PadConfig));
    }

    prefs.end();
    Serial.println("[CONFIG] Configuration saved to NVS");
    return true;
}

// ============================================================================
// GETTERS/SETTERS
// ============================================================================

PadConfig& PadConfigManager::getConfig(uint8_t padId) {
    if (padId >= 8) padId = 0;  // Safety
    return configs[padId];
}

void PadConfigManager::setConfig(uint8_t padId, const PadConfig& config) {
    if (padId >= 8) return;
    configs[padId] = config;
}

// ============================================================================
// INDIVIDUAL PARAMETER UPDATES
// ============================================================================

void PadConfigManager::setThreshold(uint8_t padId, uint16_t value) {
    if (padId >= 8) return;
    configs[padId].threshold = constrain(value, 50, 2000);
}

void PadConfigManager::setVelocityRange(uint8_t padId, uint16_t min, uint16_t max) {
    if (padId >= 8) return;
    configs[padId].velocityMin = constrain(min, 50, 1000);
    configs[padId].velocityMax = constrain(max, 500, 4000);
}

void PadConfigManager::setVelocityCurve(uint8_t padId, float curve) {
    if (padId >= 8) return;
    configs[padId].velocityCurve = constrain(curve, 0.3f, 2.0f);
}

void PadConfigManager::setMidiNote(uint8_t padId, uint8_t note) {
    if (padId >= 8) return;
    configs[padId].midiNote = (note > 127) ? 127 : note;
}

void PadConfigManager::setSample(uint8_t padId, const char* filename) {
    if (padId >= 8) return;
    strncpy(configs[padId].sampleName, filename, 31);
    configs[padId].sampleName[31] = '\0';
}

void PadConfigManager::setLEDColor(uint8_t padId, uint32_t hitColor, uint32_t idleColor) {
    if (padId >= 8) return;
    configs[padId].ledColorHit = hitColor;
    configs[padId].ledColorIdle = idleColor;
}

void PadConfigManager::setCrosstalk(uint8_t padId, bool enabled, uint16_t window, float ratio) {
    if (padId >= 8) return;
    configs[padId].crosstalkEnabled = enabled;
    configs[padId].crosstalkWindow = constrain(window, 10, 200);
    configs[padId].crosstalkRatio = constrain(ratio, 0.3f, 0.95f);
}

// ============================================================================
// BULK OPERATIONS
// ============================================================================

void PadConfigManager::resetToDefaults(uint8_t padId) {
    if (padId >= 4) return;

    switch (padId) {
        case 0: configs[0] = DEFAULT_KICK_CONFIG; break;
        case 1: configs[1] = DEFAULT_SNARE_CONFIG; break;
        case 2: configs[2] = DEFAULT_HIHAT_CONFIG; break;
        case 3: configs[3] = DEFAULT_TOM_CONFIG; break;
    }

    Serial.printf("[CONFIG] Pad %d reset to defaults\n", padId);
}

void PadConfigManager::resetAllToDefaults() {
    configs[0] = DEFAULT_KICK_CONFIG;
    configs[1] = DEFAULT_SNARE_CONFIG;
    configs[2] = DEFAULT_HIHAT_CONFIG;
    configs[3] = DEFAULT_TOM_CONFIG;

    Serial.println("[CONFIG] All pads reset to defaults");
}

// ============================================================================
// JSON EXPORT/IMPORT (for GUI communication)
// ============================================================================

String PadConfigManager::exportJSON() {
    DynamicJsonDocument doc(2048);

    for (uint8_t i = 0; i < 4; i++) {
        JsonObject pad = doc["pads"][i].to<JsonObject>();
        PadConfig& cfg = configs[i];

        // Trigger settings
        pad["threshold"] = cfg.threshold;
        pad["velocityMin"] = cfg.velocityMin;
        pad["velocityMax"] = cfg.velocityMax;
        pad["velocityCurve"] = cfg.velocityCurve;

        // Crosstalk
        pad["crosstalkEnabled"] = cfg.crosstalkEnabled;
        pad["crosstalkWindow"] = cfg.crosstalkWindow;
        pad["crosstalkRatio"] = cfg.crosstalkRatio;

        // Audio/MIDI
        pad["midiNote"] = cfg.midiNote;
        pad["midiChannel"] = cfg.midiChannel;
        pad["sampleName"] = cfg.sampleName;
        pad["sampleVolume"] = cfg.sampleVolume;

        // LED
        pad["ledColorHit"] = cfg.ledColorHit;
        pad["ledColorIdle"] = cfg.ledColorIdle;
        pad["ledBrightness"] = cfg.ledBrightness;

        // Metadata
        pad["name"] = cfg.name;
        pad["enabled"] = cfg.enabled;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool PadConfigManager::importJSON(const String& json) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.printf("[CONFIG] JSON parse error: %s\n", error.c_str());
        return false;
    }

    JsonArray pads = doc["pads"];
    if (!pads) return false;

    for (uint8_t i = 0; i < 4 && i < pads.size(); i++) {
        JsonObject pad = pads[i];
        PadConfig& cfg = configs[i];

        // Only update fields that exist in JSON
        if (pad.containsKey("threshold")) cfg.threshold = pad["threshold"];
        if (pad.containsKey("velocityMin")) cfg.velocityMin = pad["velocityMin"];
        if (pad.containsKey("velocityMax")) cfg.velocityMax = pad["velocityMax"];
        if (pad.containsKey("velocityCurve")) cfg.velocityCurve = pad["velocityCurve"];

        if (pad.containsKey("crosstalkEnabled")) cfg.crosstalkEnabled = pad["crosstalkEnabled"];
        if (pad.containsKey("crosstalkWindow")) cfg.crosstalkWindow = pad["crosstalkWindow"];
        if (pad.containsKey("crosstalkRatio")) cfg.crosstalkRatio = pad["crosstalkRatio"];

        if (pad.containsKey("midiNote")) cfg.midiNote = pad["midiNote"];
        if (pad.containsKey("midiChannel")) cfg.midiChannel = pad["midiChannel"];
        if (pad.containsKey("sampleName")) strncpy(cfg.sampleName, pad["sampleName"] | "", 31);
        if (pad.containsKey("sampleVolume")) cfg.sampleVolume = pad["sampleVolume"];

        if (pad.containsKey("ledColorHit")) cfg.ledColorHit = pad["ledColorHit"];
        if (pad.containsKey("ledColorIdle")) cfg.ledColorIdle = pad["ledColorIdle"];
        if (pad.containsKey("ledBrightness")) cfg.ledBrightness = pad["ledBrightness"];

        if (pad.containsKey("name")) strncpy(cfg.name, pad["name"] | "", 15);
        if (pad.containsKey("enabled")) cfg.enabled = pad["enabled"];
    }

    Serial.println("[CONFIG] Configuration imported from JSON");
    return true;
}
