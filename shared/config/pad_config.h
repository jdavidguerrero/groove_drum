#ifndef PAD_CONFIG_H
#define PAD_CONFIG_H

#include <Arduino.h>

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
    char sampleName[32];         // Sample filename (e.g., "kick_001.wav")
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

// Factory defaults for each pad type
const PadConfig DEFAULT_KICK_CONFIG = {
    // Trigger
    .threshold = 250,
    .velocityMin = 150,
    .velocityMax = 2500,
    .velocityCurve = 0.5f,  // Square root for natural feel

    // Crosstalk
    .crosstalkEnabled = true,
    .crosstalkWindow = 50,
    .crosstalkRatio = 0.7f,
    .crosstalkMask = 0b00001110,  // Check snare, hihat, tom (not itself)

    // Timing
    .peakWindowMs = 2,
    .decayTimeMs = 30,
    .minRetriggerMs = 20,

    // Audio/MIDI
    .midiNote = 36,  // C1 (standard kick)
    .midiChannel = 10,  // MIDI drum channel
    .sampleName = "kick_001.wav",
    .sampleVolume = 100,
    .samplePitch = 0,

    // Visual
    .ledColorHit = 0xFF0000,    // Red on hit
    .ledColorIdle = 0x330000,   // Dim red idle
    .ledBrightness = 80,
    .ledFadeDuration = 200,

    // Advanced
    .dualZoneEnabled = false,
    .rimThreshold = 0,
    .rimMidiNote = 0,
    .rimSampleName = "",

    // Metadata
    .name = "Kick",
    .padType = 0,
    .enabled = true
};

const PadConfig DEFAULT_SNARE_CONFIG = {
    .threshold = 250,
    .velocityMin = 150,
    .velocityMax = 2200,
    .velocityCurve = 0.5f,

    .crosstalkEnabled = true,
    .crosstalkWindow = 50,
    .crosstalkRatio = 0.7f,
    .crosstalkMask = 0b00001101,  // Check kick, hihat, tom

    .peakWindowMs = 2,
    .decayTimeMs = 25,
    .minRetriggerMs = 15,

    .midiNote = 38,  // D1 (standard snare)
    .midiChannel = 10,
    .sampleName = "snare_001.wav",
    .sampleVolume = 95,
    .samplePitch = 0,

    .ledColorHit = 0x00FF00,    // Green on hit
    .ledColorIdle = 0x003300,
    .ledBrightness = 80,
    .ledFadeDuration = 150,

    .dualZoneEnabled = true,     // Snare can have rim shots
    .rimThreshold = 300,
    .rimMidiNote = 40,           // E1 (rimshot)
    .rimSampleName = "snare_rim_001.wav",

    .name = "Snare",
    .padType = 1,
    .enabled = true
};

const PadConfig DEFAULT_HIHAT_CONFIG = {
    .threshold = 200,  // More sensitive
    .velocityMin = 100,
    .velocityMax = 1800,
    .velocityCurve = 0.5f,

    .crosstalkEnabled = true,
    .crosstalkWindow = 50,
    .crosstalkRatio = 0.7f,
    .crosstalkMask = 0b00001011,  // Check kick, snare, tom

    .peakWindowMs = 2,
    .decayTimeMs = 20,
    .minRetriggerMs = 10,  // Faster for rolls

    .midiNote = 42,  // F#1 (closed hihat)
    .midiChannel = 10,
    .sampleName = "hihat_closed_001.wav",
    .sampleVolume = 85,
    .samplePitch = 0,

    .ledColorHit = 0x00FFFF,    // Cyan on hit
    .ledColorIdle = 0x003333,
    .ledBrightness = 80,
    .ledFadeDuration = 100,

    .dualZoneEnabled = true,     // Closed vs Open
    .rimThreshold = 350,
    .rimMidiNote = 46,           // A#1 (open hihat)
    .rimSampleName = "hihat_open_001.wav",

    .name = "HiHat",
    .padType = 4,
    .enabled = true
};

const PadConfig DEFAULT_TOM_CONFIG = {
    .threshold = 250,
    .velocityMin = 150,
    .velocityMax = 2000,
    .velocityCurve = 0.5f,

    .crosstalkEnabled = true,
    .crosstalkWindow = 50,
    .crosstalkRatio = 0.7f,
    .crosstalkMask = 0b00000111,  // Check kick, snare, hihat

    .peakWindowMs = 2,
    .decayTimeMs = 30,
    .minRetriggerMs = 15,

    .midiNote = 48,  // C2 (high tom)
    .midiChannel = 10,
    .sampleName = "tom_001.wav",
    .sampleVolume = 90,
    .samplePitch = 0,

    .ledColorHit = 0x0000FF,    // Blue on hit
    .ledColorIdle = 0x000033,
    .ledBrightness = 80,
    .ledFadeDuration = 180,

    .dualZoneEnabled = false,
    .rimThreshold = 0,
    .rimMidiNote = 0,
    .rimSampleName = "",

    .name = "Tom",
    .padType = 2,
    .enabled = true
};

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
