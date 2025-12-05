#ifndef CALIBRATION_MANAGER_H
#define CALIBRATION_MANAGER_H

#include <Arduino.h>

// ============================================================================
// CALIBRATION MANAGER
// ============================================================================
// Automated calibration system that:
// 1. Observes baseline noise (10s)
// 2. Captures soft hits to determine velocityMin (10s)
// 3. Captures hard hits to determine velocityMax (10s)
// 4. Calculates optimal threshold and saves to NVS

namespace CalibrationManager {

enum CalibrationPhase {
    PHASE_IDLE,
    PHASE_BASELINE,      // Observing noise without hits
    PHASE_SOFT_HITS,     // User hits softly
    PHASE_HARD_HITS      // User hits hard
};

// Start calibration for a specific pad (30s total)
void startCalibration(uint8_t padId);

// Stop calibration prematurely
void stopCalibration();

// Check if calibration is active
bool isActive();

// Update calibration state (call from main loop)
void update();

// Get current calibration state
uint8_t getCurrentPad();
CalibrationPhase getCurrentPhase();

// Forward declare internal structure for GUI access
struct PadCalibrationData;
const PadCalibrationData& getPadData(uint8_t padId);

}  // namespace CalibrationManager

#endif // CALIBRATION_MANAGER_H
