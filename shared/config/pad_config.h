#ifndef PAD_CONFIG_H
#define PAD_CONFIG_H

#include <Arduino.h>
#include <cstring>
#include <edrum_config.h>

// ============================================================================
// PAD CONFIGURATION STRUCTURE
// ============================================================================
// This structure holds all configurable parameters for each drum pad.
// Can be modified at runtime via GUI and stored in NVS (Non-Volatile Storage)

struct PadConfig {
    // === TRIGGER DETECTION ===
    uint16_t threshold;          // ADC units (200-1000), noise rejection level
    uint16_t velocityMin;        // Min ADC for velocity mapping (100-500)
    uint16_t velocityMax;        // Max ADC for velocity mapping (1000-3000)
    float velocityCurve;         // Exponential curve (0.3-2.0): <1=soft emphasis, >1=hard emphasis

    // === CROSSTALK REJECTION ===
    bool crosstalkEnabled;       // Enable/disable crosstalk rejection
    uint16_t crosstalkWindow;    // Time window in ms (20-100ms)
    float crosstalkRatio;        // Velocity ratio threshold (0.5-0.95)
    uint8_t crosstalkMask;       // Bitmask of pads to check (bit 0-7 = pad 0-7)

    // === TIMING ===
    uint16_t peakWindowMs;       // Peak detection window (1-5ms)
    uint16_t decayTimeMs;        // Decay timeout (10-100ms)
    uint8_t minRetriggerMs;      // Minimum time between hits (10-50ms)

    // === AUDIO/MIDI ===
    uint8_t midiNote;            // MIDI note number (0-127)
    uint8_t midiChannel;         // MIDI channel (1-16)
    char sampleName[32];         // Sample filename (full path)
    uint8_t sampleVolume;        // Volume (0-100)
    int8_t samplePitch;          // Pitch shift in semitones (-12 to +12)

    // === VISUAL ===
    uint32_t ledColorHit;        // RGB color on hit (0xRRGGBB)
    uint32_t ledColorIdle;       // RGB color when idle
    uint8_t ledBrightness;       // LED brightness (0-100)
    uint16_t ledFadeDuration;    // Fade time in ms (50-1000)

    // === ADVANCED ===
    bool dualZoneEnabled;        // Enable dual-zone (head/rim) detection
    uint16_t rimThreshold;       // Rim threshold if dual-zone
    uint8_t rimMidiNote;         // Rim MIDI note
    char rimSampleName[32];      // Rim sample filename

    // === METADATA ===
    char name[16];               // User-defined pad name (e.g., "Snare", "Kick")
    uint8_t padType;             // 0=Kick, 1=Snare, 2=Tom, 3=Cymbal, 4=HiHat
    bool enabled;                // Enable/disable this pad completely
};

// ============================================================================
// DEFAULT CONFIGURATIONS
// ============================================================================

inline PadConfig makeKickConfig() {
    PadConfig cfg{};
    cfg.threshold = 250;
    cfg.velocityMin = 150;
    cfg.velocityMax = 2500;
    cfg.velocityCurve = 0.5f;
    cfg.crosstalkEnabled = true;
    cfg.crosstalkWindow = 50;
    cfg.crosstalkRatio = 0.7f;
    cfg.crosstalkMask = 0b00001110;
    cfg.peakWindowMs = 2;
    cfg.decayTimeMs = 30;
    cfg.minRetriggerMs = 20;
    cfg.midiNote = 36;
    cfg.midiChannel = 10;
    std::strncpy(cfg.sampleName, SAMPLE_PATH_KICK, sizeof(cfg.sampleName));
    cfg.sampleName[sizeof(cfg.sampleName) - 1] = '\0';
    cfg.sampleVolume = 100;
    cfg.samplePitch = 0;
    cfg.ledColorHit = 0xFF0000;
    cfg.ledColorIdle = 0x330000;
    cfg.ledBrightness = 80;
    cfg.ledFadeDuration = 200;
    cfg.dualZoneEnabled = false;
    cfg.rimThreshold = 0;
    cfg.rimMidiNote = 0;
    cfg.rimSampleName[0] = '\0';
    std::strncpy(cfg.name, "Kick", sizeof(cfg.name));
    cfg.name[sizeof(cfg.name) - 1] = '\0';
    cfg.padType = 0;
    cfg.enabled = true;
    return cfg;
}

inline PadConfig makeSnareConfig() {
    PadConfig cfg{};
    cfg.threshold = 250;
    cfg.velocityMin = 150;
    cfg.velocityMax = 2200;
    cfg.velocityCurve = 0.5f;
    cfg.crosstalkEnabled = true;
    cfg.crosstalkWindow = 50;
    cfg.crosstalkRatio = 0.7f;
    cfg.crosstalkMask = 0b00001101;
    cfg.peakWindowMs = 2;
    cfg.decayTimeMs = 25;
    cfg.minRetriggerMs = 15;
    cfg.midiNote = 38;
    cfg.midiChannel = 10;
    std::strncpy(cfg.sampleName, SAMPLE_PATH_SNARE, sizeof(cfg.sampleName));
    cfg.sampleName[sizeof(cfg.sampleName) - 1] = '\0';
    cfg.sampleVolume = 95;
    cfg.samplePitch = 0;
    cfg.ledColorHit = 0x00FF00;
    cfg.ledColorIdle = 0x003300;
    cfg.ledBrightness = 80;
    cfg.ledFadeDuration = 150;
    cfg.dualZoneEnabled = true;
    cfg.rimThreshold = 300;
    cfg.rimMidiNote = 40;
    std::strncpy(cfg.rimSampleName, "snare_rim_001.wav", sizeof(cfg.rimSampleName));
    cfg.rimSampleName[sizeof(cfg.rimSampleName) - 1] = '\0';
    std::strncpy(cfg.name, "Snare", sizeof(cfg.name));
    cfg.name[sizeof(cfg.name) - 1] = '\0';
    cfg.padType = 1;
    cfg.enabled = true;
    return cfg;
}

inline PadConfig makeHiHatConfig() {
    PadConfig cfg{};
    cfg.threshold = 200;
    cfg.velocityMin = 100;
    cfg.velocityMax = 1800;
    cfg.velocityCurve = 0.5f;
    cfg.crosstalkEnabled = true;
    cfg.crosstalkWindow = 50;
    cfg.crosstalkRatio = 0.7f;
    cfg.crosstalkMask = 0b00001011;
    cfg.peakWindowMs = 2;
    cfg.decayTimeMs = 20;
    cfg.minRetriggerMs = 10;
    cfg.midiNote = 42;
    cfg.midiChannel = 10;
    std::strncpy(cfg.sampleName, SAMPLE_PATH_HIHAT, sizeof(cfg.sampleName));
    cfg.sampleName[sizeof(cfg.sampleName) - 1] = '\0';
    cfg.sampleVolume = 85;
    cfg.samplePitch = 0;
    cfg.ledColorHit = 0x00FFFF;
    cfg.ledColorIdle = 0x003333;
    cfg.ledBrightness = 80;
    cfg.ledFadeDuration = 100;
    cfg.dualZoneEnabled = true;
    cfg.rimThreshold = 350;
    cfg.rimMidiNote = 46;
    std::strncpy(cfg.rimSampleName, "hihat_open_001.wav", sizeof(cfg.rimSampleName));
    cfg.rimSampleName[sizeof(cfg.rimSampleName) - 1] = '\0';
    std::strncpy(cfg.name, "HiHat", sizeof(cfg.name));
    cfg.name[sizeof(cfg.name) - 1] = '\0';
    cfg.padType = 4;
    cfg.enabled = true;
    return cfg;
}

inline PadConfig makeTomConfig() {
    PadConfig cfg{};
    cfg.threshold = 250;
    cfg.velocityMin = 150;
    cfg.velocityMax = 2000;
    cfg.velocityCurve = 0.5f;
    cfg.crosstalkEnabled = true;
    cfg.crosstalkWindow = 50;
    cfg.crosstalkRatio = 0.7f;
    cfg.crosstalkMask = 0b00000111;
    cfg.peakWindowMs = 2;
    cfg.decayTimeMs = 30;
    cfg.minRetriggerMs = 15;
    cfg.midiNote = 48;
    cfg.midiChannel = 10;
    std::strncpy(cfg.sampleName, SAMPLE_PATH_TOM, sizeof(cfg.sampleName));
    cfg.sampleName[sizeof(cfg.sampleName) - 1] = '\0';
    cfg.sampleVolume = 90;
    cfg.samplePitch = 0;
    cfg.ledColorHit = 0x0000FF;
    cfg.ledColorIdle = 0x000033;
    cfg.ledBrightness = 80;
    cfg.ledFadeDuration = 180;
    cfg.dualZoneEnabled = false;
    cfg.rimThreshold = 0;
    cfg.rimMidiNote = 0;
    cfg.rimSampleName[0] = '\0';
    std::strncpy(cfg.name, "Tom", sizeof(cfg.name));
    cfg.name[sizeof(cfg.name) - 1] = '\0';
    cfg.padType = 2;
    cfg.enabled = true;
    return cfg;
}

const PadConfig DEFAULT_KICK_CONFIG = makeKickConfig();
const PadConfig DEFAULT_SNARE_CONFIG = makeSnareConfig();
const PadConfig DEFAULT_HIHAT_CONFIG = makeHiHatConfig();
const PadConfig DEFAULT_TOM_CONFIG = makeTomConfig();

// ============================================================================
// CONFIGURATION MANAGEMENT
// ============================================================================

class PadConfigManager {
public:
    // Initialize with defaults
    static void init();

    // Load/Save to NVS (Non-Volatile Storage)
    static bool loadFromNVS();
    static bool saveToNVS();

    // Get/Set individual pad config
    static PadConfig& getConfig(uint8_t padId);
    static void setConfig(uint8_t padId, const PadConfig& config);

    // Update individual parameters (for GUI)
    static void setThreshold(uint8_t padId, uint16_t value);
    static void setVelocityRange(uint8_t padId, uint16_t min, uint16_t max);
    static void setVelocityCurve(uint8_t padId, float curve);
    static void setMidiNote(uint8_t padId, uint8_t note);
    static void setSample(uint8_t padId, const char* filename);
    static void setLEDColor(uint8_t padId, uint32_t hitColor, uint32_t idleColor);
    static void setCrosstalk(uint8_t padId, bool enabled, uint16_t window, float ratio);

    // Bulk operations
    static void resetToDefaults(uint8_t padId);
    static void resetAllToDefaults();

    // Export/Import JSON (for backup/restore)
    static String exportJSON();
    static bool importJSON(const String& json);

private:
    static PadConfig configs[8];  // Support up to 8 pads
};

#endif // PAD_CONFIG_H
