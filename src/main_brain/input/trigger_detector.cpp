/**
 * @file trigger_detector.cpp
 * @brief Implementation of trigger detection algorithm
 * @version 1.0
 * @date 2025-12-02
 */

#include "trigger_detector.h"
#include <math.h>

// Global instance
TriggerDetector triggerDetector;

// ============================================================
// CONSTRUCTOR
// ============================================================

TriggerDetector::TriggerDetector() : hitEventQueue(nullptr) {
    // Initialize all pad states
    for (int i = 0; i < NUM_PADS; i++) {
        padStates[i] = PadState();
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

void TriggerDetector::begin(QueueHandle_t hitQueue) {
    hitEventQueue = hitQueue;

    Serial.println("[TriggerDetector] Initialized");
    Serial.println("  Per-Pad Thresholds:");
    for (int i = 0; i < NUM_PADS; i++) {
        Serial.printf("    %s: %d ADC (%.2fV)\n",
                      PAD_NAMES[i],
                      TRIGGER_THRESHOLD_PER_PAD[i],
                      (TRIGGER_THRESHOLD_PER_PAD[i] * 2.45f) / 4095.0f);
    }
    Serial.printf("  Scan Time: %d µs\n", TRIGGER_SCAN_TIME_US);
    Serial.printf("  Mask Time: %d µs\n", TRIGGER_MASK_TIME_US);
    Serial.printf("  Crosstalk Window: %d µs\n", TRIGGER_CROSSTALK_WINDOW_US);
}

// ============================================================
// MAIN PROCESSING FUNCTION
// ============================================================

void TriggerDetector::processSample(uint8_t padId, uint16_t rawValue, uint32_t timestamp) {
    if (padId >= NUM_PADS) return;

    PadState& pad = padStates[padId];

    // Update baseline tracking (slow exponential moving average)
    // This tracks DC offset drift due to temperature, etc.
    updateBaseline(padId, rawValue);

    // Calculate AC signal (remove DC baseline)
    int16_t signal = (int16_t)rawValue - (int16_t)pad.baselineValue;
    if (signal < 0) signal = 0;  // Clamp to positive

    // State machine
    switch (pad.state) {
        case STATE_IDLE: {
            // Waiting for threshold crossing (per-pad threshold)
            uint16_t threshold = TRIGGER_THRESHOLD_PER_PAD[padId];

            if (signal > threshold) {
                // Threshold crossed - enter RISING state
                pad.state = STATE_RISING;
                pad.peakValue = signal;
                pad.risingStartTime = timestamp;

                #ifdef DEBUG_TRIGGER_EVENTS
                Serial.printf("[Pad %d] Threshold crossed: signal=%d (threshold=%d)\n",
                              padId, signal, threshold);
                #endif
            }
            break;
        }

        case STATE_RISING: {
            // Seeking peak value within scan time window

            // Update peak if signal is still increasing
            if (signal > pad.peakValue) {
                pad.peakValue = signal;
            }

            // Check if scan time expired OR signal dropped significantly
            uint32_t elapsed = timestamp - pad.risingStartTime;
            bool scanTimeExpired = (elapsed > TRIGGER_SCAN_TIME_US);
            bool signalDropped = (signal < (pad.peakValue * 0.7f));

            if (scanTimeExpired || signalDropped) {
                // Peak found - process hit
                pad.state = STATE_DECAY;  // Enter decay/mask state
                pad.peakTime = timestamp;

                // Convert peak to MIDI velocity
                uint8_t velocity = peakToVelocity(pad.peakValue, padId);

                // Crosstalk rejection
                if (isCrosstalk(padId, timestamp, velocity)) {
                    #ifdef DEBUG_TRIGGER_EVENTS
                    Serial.printf("[Pad %d] REJECTED (crosstalk): peak=%d, vel=%d\n",
                                  padId, pad.peakValue, velocity);
                    #endif
                } else {
                    // Valid hit - send event
                    sendHitEvent(padId, velocity, timestamp);
                    pad.lastVelocity = velocity;
                    pad.lastHitTime = timestamp;

                    #ifdef DEBUG_TRIGGER_EVENTS
                    Serial.printf("[Pad %d] HIT: peak=%d, vel=%d, time=%d µs\n",
                                  padId, pad.peakValue, velocity, elapsed);
                    #endif
                }
            }
            break;
        }

        case STATE_DECAY: {
            // Mask time - wait for signal to drop and time to pass

            uint32_t maskElapsed = timestamp - pad.peakTime;
            bool maskTimeExpired = (maskElapsed > TRIGGER_MASK_TIME_US);
            bool signalLow = (signal < TRIGGER_RETRIGGER_THRESHOLD);

            if (maskTimeExpired && signalLow) {
                // Re-arm trigger
                pad.state = STATE_IDLE;

                #ifdef DEBUG_TRIGGER_EVENTS
                Serial.printf("[Pad %d] Re-armed\n", padId);
                #endif
            }
            break;
        }

        default:
            // Unknown state - reset
            pad.state = STATE_IDLE;
            break;
    }
}

// ============================================================
// BASELINE TRACKING
// ============================================================

void TriggerDetector::updateBaseline(uint8_t padId, uint16_t rawValue) {
    PadState& pad = padStates[padId];

    // Exponential moving average: baseline = (baseline * (N-1) + raw) / N
    // Using bit shift for efficiency: N = 1024
    // baseline = (baseline * 1023 + raw) >> 10
    pad.baselineValue = (pad.baselineValue * (BASELINE_UPDATE_WEIGHT - 1) + rawValue) >> 10;
}

// ============================================================
// VELOCITY MAPPING
// ============================================================

uint8_t TriggerDetector::peakToVelocity(uint16_t peakValue, uint8_t padId) {
    // Get calibration values for this pad
    uint16_t minPeak = VELOCITY_MIN_PEAK[padId];
    uint16_t maxPeak = VELOCITY_MAX_PEAK[padId];

    // Clamp to calibrated range
    if (peakValue < minPeak) return MIDI_VELOCITY_MIN;
    if (peakValue > maxPeak) return MIDI_VELOCITY_MAX;

    // Normalize to 0.0-1.0 range
    float normalized = (float)(peakValue - minPeak) / (float)(maxPeak - minPeak);

    // Apply velocity curve
    // Exponent < 1.0 = compression (easier to reach high velocities)
    // Exponent > 1.0 = expansion (harder to reach high velocities)
    // 0.5 = square root (natural drum feel)
    float curved = pow(normalized, VELOCITY_CURVE_EXPONENT);

    // Map to MIDI velocity range (1-127)
    // Note: 0 is reserved for note-off in MIDI
    uint8_t velocity = (uint8_t)(curved * (MIDI_VELOCITY_MAX - MIDI_VELOCITY_MIN)) + MIDI_VELOCITY_MIN;

    // Clamp to valid range
    velocity = CLAMP(velocity, MIDI_VELOCITY_MIN, MIDI_VELOCITY_MAX);

    return velocity;
}

// ============================================================
// CROSSTALK REJECTION
// ============================================================

bool TriggerDetector::isCrosstalk(uint8_t currentPad, uint32_t timestamp, uint8_t velocity) {
    // Check if any OTHER pad hit very recently
    for (uint8_t otherPad = 0; otherPad < NUM_PADS; otherPad++) {
        if (otherPad == currentPad) continue;

        uint32_t timeSinceOtherHit = timestamp - padStates[otherPad].lastHitTime;

        if (timeSinceOtherHit < TRIGGER_CROSSTALK_WINDOW_US) {
            // Another pad hit within crosstalk window
            // Check velocity ratio
            uint8_t otherVelocity = padStates[otherPad].lastVelocity;

            if (velocity < (otherVelocity * TRIGGER_CROSSTALK_RATIO)) {
                // Current hit is significantly weaker - likely crosstalk
                #ifdef DEBUG_TRIGGER_EVENTS
                Serial.printf("[Crosstalk] Pad %d vel=%d < Pad %d vel=%d * %.2f\n",
                              currentPad, velocity, otherPad, otherVelocity,
                              TRIGGER_CROSSTALK_RATIO);
                #endif
                return true;
            }
        }
    }

    return false;  // Not crosstalk
}

// ============================================================
// EVENT SENDING
// ============================================================

void TriggerDetector::sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp) {
    if (hitEventQueue == nullptr) {
        Serial.println("[ERROR] Hit event queue not initialized!");
        return;
    }

    HitEvent event(padId, velocity, timestamp);

    // Send to queue (don't block if queue is full)
    BaseType_t result = xQueueSend(hitEventQueue, &event, 0);

    if (result != pdPASS) {
        Serial.printf("[WARNING] Hit event queue full! Lost event from Pad %d\n", padId);
    }

    #ifdef DEBUG_TRIGGER_EVENTS
    Serial.printf("[HitEvent] Pad=%d, Vel=%d, Time=%lu\n", padId, velocity, timestamp);
    #endif
}

// ============================================================
// STATE QUERY FUNCTIONS
// ============================================================

TriggerState TriggerDetector::getState(uint8_t padId) const {
    if (padId >= NUM_PADS) return STATE_IDLE;
    return padStates[padId].state;
}

uint16_t TriggerDetector::getBaseline(uint8_t padId) const {
    if (padId >= NUM_PADS) return 0;
    return padStates[padId].baselineValue;
}

const PadState& TriggerDetector::getPadState(uint8_t padId) const {
    static PadState dummy;
    if (padId >= NUM_PADS) return dummy;
    return padStates[padId];
}

// ============================================================
// RESET FUNCTIONS
// ============================================================

void TriggerDetector::resetPad(uint8_t padId) {
    if (padId >= NUM_PADS) return;
    padStates[padId] = PadState();
    Serial.printf("[TriggerDetector] Pad %d reset\n", padId);
}

void TriggerDetector::resetAll() {
    for (int i = 0; i < NUM_PADS; i++) {
        resetPad(i);
    }
    Serial.println("[TriggerDetector] All pads reset");
}

// ============================================================
// DEBUG OUTPUT
// ============================================================

void TriggerDetector::printState() const {
    Serial.println("--- Trigger Detector State ---");
    for (int i = 0; i < NUM_PADS; i++) {
        const PadState& pad = padStates[i];

        const char* stateName;
        switch (pad.state) {
            case STATE_IDLE:          stateName = "IDLE";          break;
            case STATE_RISING:        stateName = "RISING";        break;
            case STATE_PEAK_DETECTED: stateName = "PEAK_DETECTED"; break;
            case STATE_DECAY:         stateName = "DECAY";         break;
            default:                  stateName = "UNKNOWN";       break;
        }

        Serial.printf("Pad %d (%s):\n", i, PAD_NAMES[i]);
        Serial.printf("  State: %s\n", stateName);
        Serial.printf("  Baseline: %d\n", pad.baselineValue);
        Serial.printf("  Peak: %d\n", pad.peakValue);
        Serial.printf("  Last Velocity: %d\n", pad.lastVelocity);
    }
    Serial.println("------------------------------");
}
