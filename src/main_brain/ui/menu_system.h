#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>
#include <vector>
#include <edrum_config.h>

// ============================================================================
// MENU SYSTEM - PAD CONFIGURATION UI
// ============================================================================
// Controls:
//   - BTN_MENU (GPIO14): Enter/Exit menu mode
//   - Encoder Left: Navigate options / Adjust values
//   - Encoder Left Switch (GPIO9): Select/Confirm
//   - BTN_EDIT (GPIO8): Switch between pads
//   - BTN_FX (GPIO38): Save configuration
//   - BTN_CLICK (GPIO15): Cancel/Back

namespace MenuSystem {

// Menu states
enum MenuState {
    MENU_HIDDEN,           // Normal playing mode
    MENU_PAD_SELECT,       // Select which pad to configure
    MENU_PAD_CONFIG,       // Configure selected pad (threshold, sensitivity, sample)
    MENU_SAMPLE_BROWSE,    // Browse samples on SD for selected pad
    MENU_SAVING            // Saving configuration
};

// Config options within pad config
enum ConfigOption {
    CONFIG_SAMPLE,         // Sample selection
    CONFIG_THRESHOLD,      // Trigger threshold
    CONFIG_SENSITIVITY,    // Velocity sensitivity (min peak)
    CONFIG_MAX_PEAK,       // Maximum peak for velocity
    CONFIG_COUNT           // Number of options
};

// Sample info from SD
struct SampleInfo {
    char path[64];
    char displayName[24];
};

// Current menu state
struct MenuContext {
    MenuState state;
    uint8_t selectedPad;          // 0-3
    ConfigOption selectedOption;  // Current option in pad config
    int16_t optionValue;          // Current value being edited
    bool editing;                 // True if currently editing a value

    // Sample browser
    std::vector<SampleInfo> availableSamples;
    uint8_t sampleScrollOffset;
    uint8_t selectedSampleIndex;

    // Display
    bool needsRedraw;
    uint32_t lastInteractionMs;

    // Unsaved changes
    bool hasChanges;
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize menu system
void begin();

// Update menu (call from loop)
void update();

// Check if menu is active (blocks normal pad display)
bool isActive();

// Get current state for display
const MenuContext& getContext();

// Force redraw
void requestRedraw();

// Input handlers (called from main input processing)
void onEncoderRotate(int8_t direction);  // +1 CW, -1 CCW
void onEncoderPress();
void onButtonMenu();     // Toggle menu
void onButtonEdit();     // Switch pads / Edit mode
void onButtonFX();       // Save
void onButtonClick();    // Cancel/Back

// Load available samples from SD
void scanSamplesFromSD();

// Save current configuration to SD
bool saveConfiguration();

// Load configuration from SD
bool loadConfiguration();

}  // namespace MenuSystem

#endif  // MENU_SYSTEM_H
