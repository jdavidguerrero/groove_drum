#include "menu_system.h"
#include "../communication/uart_protocol.h"
#include "../output/audio_samples.h"
#include "pad_config.h"
#include <SD.h>

namespace MenuSystem {

// Forward declarations
static void scanDirectory(File& dir, uint8_t depth);
static void sendDisplayUpdate();

// ============================================================================
// STATE
// ============================================================================

static MenuContext ctx;

// Option names for display
static const char* OPTION_NAMES[CONFIG_COUNT] = {
    "SAMPLE",
    "THRESHOLD",
    "SENSITIVITY",
    "MAX PEAK"
};

// Timeout for auto-exit (30 seconds)
static const uint32_t MENU_TIMEOUT_MS = 30000;

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    ctx.state = MENU_HIDDEN;
    ctx.selectedPad = 0;
    ctx.selectedOption = CONFIG_SAMPLE;
    ctx.optionValue = 0;
    ctx.editing = false;
    ctx.sampleScrollOffset = 0;
    ctx.selectedSampleIndex = 0;
    ctx.needsRedraw = false;
    ctx.lastInteractionMs = 0;
    ctx.hasChanges = false;
    ctx.availableSamples.clear();

    // Try to load saved config
    loadConfiguration();

    Serial.println("[MENU] Menu system initialized");
}

// ============================================================================
// UPDATE
// ============================================================================

void update() {
    if (ctx.state == MENU_HIDDEN) return;

    // Auto-timeout
    if (millis() - ctx.lastInteractionMs > MENU_TIMEOUT_MS) {
        Serial.println("[MENU] Timeout - returning to play mode");
        ctx.state = MENU_HIDDEN;
        ctx.needsRedraw = true;
        sendDisplayUpdate();
        return;
    }

    // Send display updates if needed
    if (ctx.needsRedraw) {
        sendDisplayUpdate();
        ctx.needsRedraw = false;
    }
}

bool isActive() {
    return ctx.state != MENU_HIDDEN;
}

const MenuContext& getContext() {
    return ctx;
}

void requestRedraw() {
    ctx.needsRedraw = true;
}

// ============================================================================
// INPUT HANDLERS
// ============================================================================

void onEncoderRotate(int8_t direction) {
    ctx.lastInteractionMs = millis();

    switch (ctx.state) {
        case MENU_HIDDEN:
            // Ignored
            break;

        case MENU_PAD_SELECT:
            // Navigate between pads
            ctx.selectedPad = (ctx.selectedPad + direction + NUM_PADS) % NUM_PADS;
            ctx.needsRedraw = true;
            Serial.printf("[MENU] Selected pad: %d (%s)\n", ctx.selectedPad, PAD_NAMES[ctx.selectedPad]);
            break;

        case MENU_PAD_CONFIG:
            if (ctx.editing) {
                // Adjust current value
                PadConfig& cfg = PadConfigManager::getConfig(ctx.selectedPad);
                int step = (direction > 0) ? 10 : -10;

                switch (ctx.selectedOption) {
                    case CONFIG_THRESHOLD:
                        cfg.threshold = constrain((int)cfg.threshold + step, 50, 1000);
                        Serial.printf("[MENU] Threshold: %d\n", cfg.threshold);
                        break;
                    case CONFIG_SENSITIVITY:
                        cfg.velocityMin = constrain((int)cfg.velocityMin + step, 50, 500);
                        Serial.printf("[MENU] Sensitivity: %d\n", cfg.velocityMin);
                        break;
                    case CONFIG_MAX_PEAK:
                        cfg.velocityMax = constrain((int)cfg.velocityMax + step * 10, 500, 4000);
                        Serial.printf("[MENU] Max Peak: %d\n", cfg.velocityMax);
                        break;
                    default:
                        break;
                }
                ctx.hasChanges = true;
            } else {
                // Navigate options
                int newOption = (int)ctx.selectedOption + direction;
                if (newOption < 0) newOption = CONFIG_COUNT - 1;
                if (newOption >= CONFIG_COUNT) newOption = 0;
                ctx.selectedOption = (ConfigOption)newOption;
                Serial.printf("[MENU] Option: %s\n", OPTION_NAMES[ctx.selectedOption]);
            }
            ctx.needsRedraw = true;
            break;

        case MENU_SAMPLE_BROWSE:
            // Navigate sample list
            if (!ctx.availableSamples.empty()) {
                int newIdx = ctx.selectedSampleIndex + direction;
                if (newIdx < 0) newIdx = ctx.availableSamples.size() - 1;
                if (newIdx >= (int)ctx.availableSamples.size()) newIdx = 0;
                ctx.selectedSampleIndex = newIdx;

                // Update scroll offset for display
                if (ctx.selectedSampleIndex < ctx.sampleScrollOffset) {
                    ctx.sampleScrollOffset = ctx.selectedSampleIndex;
                } else if (ctx.selectedSampleIndex >= ctx.sampleScrollOffset + 4) {
                    ctx.sampleScrollOffset = ctx.selectedSampleIndex - 3;
                }

                Serial.printf("[MENU] Sample: %s\n", ctx.availableSamples[ctx.selectedSampleIndex].displayName);
            }
            ctx.needsRedraw = true;
            break;

        default:
            break;
    }
}

void onEncoderPress() {
    ctx.lastInteractionMs = millis();

    switch (ctx.state) {
        case MENU_HIDDEN:
            // Enter menu
            ctx.state = MENU_PAD_SELECT;
            ctx.needsRedraw = true;
            Serial.println("[MENU] Entering menu - Pad Select");
            break;

        case MENU_PAD_SELECT:
            // Enter pad config
            ctx.state = MENU_PAD_CONFIG;
            ctx.selectedOption = CONFIG_SAMPLE;
            ctx.editing = false;
            ctx.needsRedraw = true;
            Serial.printf("[MENU] Configuring pad %d\n", ctx.selectedPad);
            break;

        case MENU_PAD_CONFIG:
            if (ctx.selectedOption == CONFIG_SAMPLE) {
                // Enter sample browser
                scanSamplesFromSD();
                ctx.state = MENU_SAMPLE_BROWSE;
                ctx.selectedSampleIndex = 0;
                ctx.sampleScrollOffset = 0;
                ctx.needsRedraw = true;
                Serial.println("[MENU] Entering sample browser");
            } else {
                // Toggle edit mode for other options
                ctx.editing = !ctx.editing;
                ctx.needsRedraw = true;
                Serial.printf("[MENU] Edit mode: %s\n", ctx.editing ? "ON" : "OFF");
            }
            break;

        case MENU_SAMPLE_BROWSE:
            // Select sample
            if (!ctx.availableSamples.empty()) {
                PadConfig& cfg = PadConfigManager::getConfig(ctx.selectedPad);
                const char* newSample = ctx.availableSamples[ctx.selectedSampleIndex].path;

                // Load new sample into memory
                if (SampleManager::loadSample(newSample)) {
                    strncpy(cfg.sampleName, newSample, sizeof(cfg.sampleName) - 1);
                    cfg.sampleName[sizeof(cfg.sampleName) - 1] = '\0';
                    ctx.hasChanges = true;
                    Serial.printf("[MENU] PAD%d sample changed to: %s\n", ctx.selectedPad + 1, cfg.sampleName);
                } else {
                    Serial.printf("[MENU] Failed to load sample: %s\n", newSample);
                }
            }
            ctx.state = MENU_PAD_CONFIG;
            ctx.needsRedraw = true;
            break;

        default:
            break;
    }
}

void onButtonMenu() {
    ctx.lastInteractionMs = millis();

    if (ctx.state == MENU_HIDDEN) {
        // Enter menu
        ctx.state = MENU_PAD_SELECT;
        ctx.selectedPad = 0;
        ctx.needsRedraw = true;
        Serial.println("[MENU] *** MENU OPENED ***");
    } else {
        // Exit menu - discard unsaved changes
        if (ctx.hasChanges) {
            Serial.println("[MENU] Discarding unsaved changes");
            // Reload config to discard changes
            PadConfigManager::loadFromNVS();
            ctx.hasChanges = false;
        }
        ctx.state = MENU_HIDDEN;
        ctx.needsRedraw = true;
        sendDisplayUpdate();
        Serial.println("[MENU] *** MENU CLOSED ***");
    }
}

void onButtonEdit() {
    ctx.lastInteractionMs = millis();

    if (ctx.state == MENU_PAD_SELECT || ctx.state == MENU_PAD_CONFIG) {
        // Quick switch between pads
        ctx.selectedPad = (ctx.selectedPad + 1) % NUM_PADS;
        ctx.needsRedraw = true;
        Serial.printf("[MENU] Quick switch to pad %d (%s)\n", ctx.selectedPad, PAD_NAMES[ctx.selectedPad]);
    }
}

void onButtonFX() {
    ctx.lastInteractionMs = millis();

    if (ctx.state != MENU_HIDDEN && ctx.hasChanges) {
        bool savedOk = true;

        // Save to NVS (flash) - persistent across reboots
        if (!PadConfigManager::saveToNVS()) {
            Serial.println("[MENU] ❌ NVS save FAILED!");
            savedOk = false;
        }

        // Save to SD card (backup)
        if (!saveConfiguration()) {
            Serial.println("[MENU] ⚠️ SD save FAILED (backup only)");
            // Don't fail if SD fails - NVS is primary
        }

        if (savedOk) {
            Serial.println("[MENU] ✅ Configuration SAVED to NVS!");
            ctx.hasChanges = false;

            // Send SAVING state to display (triggers toast)
            ctx.state = MENU_SAVING;
            sendDisplayUpdate();

            // Return to home (Performance screen)
            ctx.state = MENU_HIDDEN;
            sendDisplayUpdate();
            Serial.println("[MENU] *** MENU CLOSED (saved) ***");
        } else {
            // Stay in config screen if save failed
            ctx.state = MENU_PAD_CONFIG;
            ctx.needsRedraw = true;
        }
    }
}

void onButtonClick() {
    ctx.lastInteractionMs = millis();

    switch (ctx.state) {
        case MENU_PAD_CONFIG:
            if (ctx.editing) {
                // Cancel editing
                ctx.editing = false;
            } else {
                // Go back to pad select
                ctx.state = MENU_PAD_SELECT;
            }
            ctx.needsRedraw = true;
            break;

        case MENU_SAMPLE_BROWSE:
            // Cancel sample selection
            ctx.state = MENU_PAD_CONFIG;
            ctx.needsRedraw = true;
            break;

        case MENU_PAD_SELECT:
            // Exit menu
            ctx.state = MENU_HIDDEN;
            ctx.needsRedraw = true;
            break;

        default:
            break;
    }
}

// ============================================================================
// SAMPLE SCANNING
// ============================================================================

void scanSamplesFromSD() {
    ctx.availableSamples.clear();

    File root = SD.open("/samples");
    if (!root || !root.isDirectory()) {
        Serial.println("[MENU] Cannot open /samples directory");
        // Add default paths anyway
        for (int i = 0; i < NUM_PADS; i++) {
            SampleInfo info;
            const char* paths[] = {SAMPLE_PATH_KICK, SAMPLE_PATH_SNARE, SAMPLE_PATH_HIHAT, SAMPLE_PATH_TOM};
            strncpy(info.path, paths[i], sizeof(info.path));
            // Extract filename
            const char* lastSlash = strrchr(paths[i], '/');
            strncpy(info.displayName, lastSlash ? lastSlash + 1 : paths[i], sizeof(info.displayName));
            ctx.availableSamples.push_back(info);
        }
        return;
    }

    // Scan recursively for .wav files
    scanDirectory(root, 0);
    root.close();

    Serial.printf("[MENU] Found %d samples\n", ctx.availableSamples.size());
}

static void scanDirectory(File& dir, uint8_t depth) {
    if (depth > 2) return;  // Max 2 levels deep

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        if (entry.isDirectory()) {
            scanDirectory(entry, depth + 1);
        } else {
            String name = entry.name();
            if (name.endsWith(".wav") || name.endsWith(".WAV")) {
                SampleInfo info;
                String fullPath = String(dir.path()) + "/" + name;
                strncpy(info.path, fullPath.c_str(), sizeof(info.path));

                // Create display name (remove extension, truncate)
                name.replace(".wav", "");
                name.replace(".WAV", "");
                if (name.length() > 20) {
                    name = name.substring(0, 17) + "...";
                }
                strncpy(info.displayName, name.c_str(), sizeof(info.displayName));

                ctx.availableSamples.push_back(info);
            }
        }
        entry.close();
    }
}

// ============================================================================
// CONFIGURATION PERSISTENCE
// ============================================================================

bool saveConfiguration() {
    // Save to /config/pads.cfg on SD
    if (!SD.exists("/config")) {
        SD.mkdir("/config");
    }

    File f = SD.open("/config/pads.cfg", FILE_WRITE);
    if (!f) {
        Serial.println("[MENU] Cannot create config file");
        return false;
    }

    // Write header
    f.println("# E-Drum Pad Configuration");
    f.println("# Format: pad,threshold,velMin,velMax,sample");

    for (uint8_t i = 0; i < NUM_PADS; i++) {
        PadConfig& cfg = PadConfigManager::getConfig(i);
        f.printf("%d,%d,%d,%d,%s\n",
                 i, cfg.threshold, cfg.velocityMin, cfg.velocityMax, cfg.sampleName);
    }

    f.close();
    Serial.println("[MENU] Configuration saved to /config/pads.cfg");
    return true;
}

bool loadConfiguration() {
    File f = SD.open("/config/pads.cfg", FILE_READ);
    if (!f) {
        Serial.println("[MENU] No saved configuration found");
        return false;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();

        // Skip comments and empty lines
        if (line.length() == 0 || line.startsWith("#")) continue;

        // Parse: pad,threshold,velMin,velMax,sample
        int pad, threshold, velMin, velMax;
        char sample[64] = {0};

        int parsed = sscanf(line.c_str(), "%d,%d,%d,%d,%63s",
                            &pad, &threshold, &velMin, &velMax, sample);

        if (parsed >= 4 && pad >= 0 && pad < NUM_PADS) {
            PadConfig& cfg = PadConfigManager::getConfig(pad);
            cfg.threshold = threshold;
            cfg.velocityMin = velMin;
            cfg.velocityMax = velMax;
            if (parsed == 5 && strlen(sample) > 0) {
                strncpy(cfg.sampleName, sample, sizeof(cfg.sampleName));
            }
            Serial.printf("[MENU] Loaded pad %d: thr=%d, sens=%d, max=%d, sample=%s\n",
                          pad, threshold, velMin, velMax, cfg.sampleName);
        }
    }

    f.close();
    return true;
}

// ============================================================================
// DISPLAY UPDATE (via UART to display MCU)
// ============================================================================

static void sendDisplayUpdate() {
    // Build MenuStateMsg with current state
    MenuStateMsg msg;
    memset(&msg, 0, sizeof(msg));

    msg.state = (uint8_t)ctx.state;
    msg.selectedPad = ctx.selectedPad;
    msg.selectedOption = (uint8_t)ctx.selectedOption;
    msg.editing = ctx.editing ? 1 : 0;
    msg.hasChanges = ctx.hasChanges ? 1 : 0;

    // Copy pad name
    strncpy(msg.padName, PAD_NAMES[ctx.selectedPad], sizeof(msg.padName) - 1);

    // Copy option name
    strncpy(msg.optionName, OPTION_NAMES[ctx.selectedOption], sizeof(msg.optionName) - 1);

    // Get current value based on selected option
    PadConfig& cfg = PadConfigManager::getConfig(ctx.selectedPad);
    switch (ctx.selectedOption) {
        case CONFIG_SAMPLE:
            msg.currentValue = 0;  // Not applicable for sample
            strncpy(msg.sampleName, cfg.sampleName, sizeof(msg.sampleName) - 1);
            break;
        case CONFIG_THRESHOLD:
            msg.currentValue = cfg.threshold;
            break;
        case CONFIG_SENSITIVITY:
            msg.currentValue = cfg.velocityMin;
            break;
        case CONFIG_MAX_PEAK:
            msg.currentValue = cfg.velocityMax;
            break;
        default:
            msg.currentValue = 0;
            break;
    }

    // Send menu state
    UARTProtocol::sendMenuState(msg);

    // If in sample browse mode, also send sample list
    if (ctx.state == MENU_SAMPLE_BROWSE && !ctx.availableSamples.empty()) {
        SampleListMsg sampleMsg;
        memset(&sampleMsg, 0, sizeof(sampleMsg));

        sampleMsg.totalCount = ctx.availableSamples.size();
        sampleMsg.startIndex = ctx.sampleScrollOffset;

        // Send up to 4 visible samples
        uint8_t visibleCount = min((size_t)4, ctx.availableSamples.size() - ctx.sampleScrollOffset);
        sampleMsg.count = visibleCount;

        for (uint8_t i = 0; i < visibleCount; i++) {
            uint8_t idx = ctx.sampleScrollOffset + i;
            sampleMsg.samples[i].index = idx;
            sampleMsg.samples[i].selected = (idx == ctx.selectedSampleIndex) ? 1 : 0;
            strncpy(sampleMsg.samples[i].displayName,
                    ctx.availableSamples[idx].displayName,
                    sizeof(sampleMsg.samples[i].displayName) - 1);
            strncpy(sampleMsg.samples[i].path,
                    ctx.availableSamples[idx].path,
                    sizeof(sampleMsg.samples[i].path) - 1);
        }

        UARTProtocol::sendSampleList(sampleMsg);
    }

    // Debug output
    Serial.printf("[MENU] State: %d, Pad: %s, Option: %s, Value: %d, Editing: %d\n",
                  ctx.state, msg.padName, msg.optionName, msg.currentValue, msg.editing);
}

}  // namespace MenuSystem
