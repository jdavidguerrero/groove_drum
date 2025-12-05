#include "calibration_manager.h"
#include "pad_config.h"
#include "trigger_detector.h"
#include "uart_protocol.h"

// ============================================================================
// CALIBRATION MANAGER - AUTO-TUNE THRESHOLDS & VELOCITY
// ============================================================================

namespace CalibrationManager {

// State
static bool isCalibrating = false;
static uint32_t calibrationStartTime = 0;
static uint8_t currentPad = 0;
static CalibrationPhase currentPhase = PHASE_IDLE;

// Statistics per pad
struct PadCalibrationData {
    uint16_t baselineSum;
    uint16_t baselineCount;
    uint16_t noiseMin;
    uint16_t noiseMax;
    uint16_t softHitMin;
    uint16_t softHitMax;
    uint16_t hardHitMax;
    uint32_t hitCount;
};

static PadCalibrationData padData[4];

// ============================================================================
// START/STOP CALIBRATION
// ============================================================================

void startCalibration(uint8_t padId) {
    if (padId >= 4) {
        Serial.println("[CALIB] Invalid pad ID");
        return;
    }

    isCalibrating = true;
    currentPad = padId;
    currentPhase = PHASE_BASELINE;
    calibrationStartTime = millis();

    // Reset statistics
    memset(&padData[padId], 0, sizeof(PadCalibrationData));
    padData[padId].noiseMin = 4095;
    padData[padId].softHitMin = 4095;

    Serial.printf("\n╔════════════════════════════════════════╗\n");
    Serial.printf("║   CALIBRATION STARTED - PAD %d         ║\n", padId);
    Serial.printf("╚════════════════════════════════════════╝\n");
    Serial.println("\nPHASE 1/3: BASELINE OBSERVATION (10s)");
    Serial.println("→ DO NOT touch the pad, let it rest.");
    Serial.println("  Observing environmental noise...\n");
}

void stopCalibration() {
    if (!isCalibrating) return;

    isCalibrating = false;
    currentPhase = PHASE_IDLE;

    Serial.println("\n[CALIB] Calibration stopped");
}

bool isActive() {
    return isCalibrating;
}

// ============================================================================
// UPDATE (call from main loop)
// ============================================================================

void update() {
    if (!isCalibrating) return;

    uint32_t elapsed = millis() - calibrationStartTime;
    PadCalibrationData& data = padData[currentPad];

    // Get current detector state
    const TriggerDetector::PadState& state = TriggerDetector::getPadState(currentPad);

    // Phase transitions
    switch (currentPhase) {
        case PHASE_BASELINE:
            // Collect baseline for 10 seconds
            data.baselineSum += state.baseline;
            data.baselineCount++;

            uint16_t noise = abs((int16_t)state.signal);
            if (noise < data.noiseMin) data.noiseMin = noise;
            if (noise > data.noiseMax) data.noiseMax = noise;

            if (elapsed > 10000) {
                // Transition to soft hits
                currentPhase = PHASE_SOFT_HITS;
                calibrationStartTime = millis();

                uint16_t avgBaseline = data.baselineSum / data.baselineCount;
                uint16_t noisePP = data.noiseMax - data.noiseMin;

                Serial.printf("\n✓ Baseline: %d | Noise: ±%d (peak-to-peak)\n",
                              avgBaseline, noisePP);
                Serial.println("\nPHASE 2/3: SOFT HITS (10s)");
                Serial.println("→ Hit the pad SOFTLY 5-10 times");
                Serial.println("  Finding minimum sensitivity...\n");
            } else {
                // Print progress every 2s
                if (elapsed % 2000 < 50) {
                    uint16_t avgBaseline = data.baselineSum / data.baselineCount;
                    Serial.printf("  Baseline: %4d | Noise: %3d-%3d | Time: %lus\n",
                                  avgBaseline, data.noiseMin, data.noiseMax, elapsed / 1000);
                }
            }
            break;

        case PHASE_SOFT_HITS:
            // Detect soft hits
            if (state.state == TriggerDetector::STATE_PEAK_DETECTED) {
                data.hitCount++;
                if (state.peakValue < data.softHitMin) {
                    data.softHitMin = state.peakValue;
                }
                if (state.peakValue > data.softHitMax) {
                    data.softHitMax = state.peakValue;
                }
                Serial.printf("  Soft hit #%lu: peak=%d\n", data.hitCount, state.peakValue);
            }

            if (elapsed > 10000) {
                // Transition to hard hits
                currentPhase = PHASE_HARD_HITS;
                calibrationStartTime = millis();

                Serial.printf("\n✓ Soft hits range: %d - %d\n",
                              data.softHitMin, data.softHitMax);
                Serial.println("\nPHASE 3/3: HARD HITS (10s)");
                Serial.println("→ Hit the pad as HARD as you can 5-10 times");
                Serial.println("  Finding maximum velocity...\n");

                data.hitCount = 0;  // Reset for hard hits
            }
            break;

        case PHASE_HARD_HITS:
            // Detect hard hits
            if (state.state == TriggerDetector::STATE_PEAK_DETECTED) {
                data.hitCount++;
                if (state.peakValue > data.hardHitMax) {
                    data.hardHitMax = state.peakValue;
                }
                Serial.printf("  Hard hit #%lu: peak=%d\n", data.hitCount, state.peakValue);
            }

            if (elapsed > 10000) {
                // FINISH CALIBRATION
                finishCalibration();
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// FINISH & APPLY RESULTS
// ============================================================================

void finishCalibration() {
    PadCalibrationData& data = padData[currentPad];
    PadConfig& cfg = PadConfigManager::getConfig(currentPad);

    // Calculate suggested values
    uint16_t avgBaseline = data.baselineSum / data.baselineCount;
    uint16_t noisePP = data.noiseMax - data.noiseMin;

    // Threshold = baseline + noise + 80 ADC margin
    uint16_t suggestedThreshold = avgBaseline + noisePP + 80;

    // Velocity range = soft hit minimum to hard hit maximum
    uint16_t suggestedVelocityMin = data.softHitMin;
    uint16_t suggestedVelocityMax = data.hardHitMax;

    // Apply safety limits
    suggestedThreshold = constrain(suggestedThreshold, 100, 1000);
    suggestedVelocityMin = constrain(suggestedVelocityMin, 50, 500);
    suggestedVelocityMax = constrain(suggestedVelocityMax, 500, 4000);

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   CALIBRATION COMPLETE                 ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    Serial.println("RESULTS:");
    Serial.printf("  Baseline:          %d ADC\n", avgBaseline);
    Serial.printf("  Noise (peak-peak): %d ADC\n", noisePP);
    Serial.printf("  Soft hit range:    %d - %d ADC\n", data.softHitMin, data.softHitMax);
    Serial.printf("  Hard hit max:      %d ADC\n", data.hardHitMax);
    Serial.println("\nSUGGESTED CONFIG:");
    Serial.printf("  Threshold:         %d ADC (was %d)\n",
                  suggestedThreshold, cfg.threshold);
    Serial.printf("  Velocity min:      %d ADC (was %d)\n",
                  suggestedVelocityMin, cfg.velocityMin);
    Serial.printf("  Velocity max:      %d ADC (was %d)\n",
                  suggestedVelocityMax, cfg.velocityMax);

    // AUTO-APPLY (you can add confirmation prompt if needed)
    cfg.threshold = suggestedThreshold;
    cfg.velocityMin = suggestedVelocityMin;
    cfg.velocityMax = suggestedVelocityMax;

    // SAVE TO NVS
    if (PadConfigManager::saveToNVS()) {
        Serial.println("\n✓ Configuration saved to NVS");
    } else {
        Serial.println("\n✗ Failed to save to NVS");
    }

    // Send to GUI
    UARTProtocol::sendConfigUpdate(currentPad);

    // Cleanup
    isCalibrating = false;
    currentPhase = PHASE_IDLE;

    Serial.println("\nCalibration finished. New settings active.\n");
}

// ============================================================================
// GETTERS
// ============================================================================

uint8_t getCurrentPad() {
    return currentPad;
}

CalibrationPhase getCurrentPhase() {
    return currentPhase;
}

const PadCalibrationData& getPadData(uint8_t padId) {
    static PadCalibrationData emptyData = {};
    if (padId >= 4) return emptyData;
    return padData[padId];
}

}  // namespace CalibrationManager
