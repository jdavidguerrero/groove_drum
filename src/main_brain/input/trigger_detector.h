/**
 * @file trigger_detector.h
 * @brief Piezo trigger detection algorithm with peak detection and crosstalk rejection
 * @version 1.0
 * @date 2025-12-02
 *
 * This module implements a state machine-based trigger detection algorithm:
 * - IDLE: Waiting for threshold crossing
 * - RISING: Seeking peak value within scan time window
 * - PEAK_DETECTED: Peak found, event sent
 * - DECAY: Mask time to prevent retriggering
 *
 * Features:
 * - Velocity-sensitive detection (MIDI 1-127)
 * - Logarithmic velocity curve (natural feel)
 * - Crosstalk rejection between adjacent pads
 * - Adaptive baseline tracking
 * - Retrigger suppression
 */

#pragma once

#include <Arduino.h>
#include <edrum_config.h>

// ============================================================
// TRIGGER STATE MACHINE
// ============================================================

/**
 * @brief Trigger detection states
 */
enum TriggerState {
    STATE_IDLE,           // Waiting for threshold crossing
    STATE_RISING,         // Above threshold, seeking peak
    STATE_PEAK_DETECTED,  // Peak found, in retrigger mask window
    STATE_DECAY           // Waiting for signal to drop below retrigger threshold
};

/**
 * @brief State data for each pad
 */
struct PadState {
    TriggerState state;       // Current state
    uint16_t peakValue;       // Peak ADC value in current hit
    uint32_t peakTime;        // Timestamp of peak detection (micros)
    uint32_t lastHitTime;     // Timestamp of last valid hit (micros)
    uint16_t baselineValue;   // DC baseline (exponential moving average)
    uint8_t lastVelocity;     // Velocity of last hit (for crosstalk rejection)
    uint32_t risingStartTime; // Timestamp when RISING state started (micros)

    PadState() :
        state(STATE_IDLE),
        peakValue(0),
        peakTime(0),
        lastHitTime(0),
        baselineValue(BASELINE_INITIAL_VALUE),
        lastVelocity(0),
        risingStartTime(0) {}
};

/**
 * @brief Hit event data structure
 */
struct HitEvent {
    uint8_t padId;
    uint8_t velocity;
    uint32_t timestamp;

    HitEvent() : padId(0), velocity(0), timestamp(0) {}
    HitEvent(uint8_t id, uint8_t vel, uint32_t time) :
        padId(id), velocity(vel), timestamp(time) {}
};

// ============================================================
// TRIGGER DETECTOR CLASS
// ============================================================

class TriggerDetector {
public:
    /**
     * @brief Constructor
     */
    TriggerDetector();

    /**
     * @brief Initialize the trigger detector
     * @param hitQueue FreeRTOS queue to send hit events to
     */
    void begin(QueueHandle_t hitQueue);

    /**
     * @brief Process a single ADC sample
     * This is the main entry point called by trigger scanner
     * @param padId Pad ID (0-3)
     * @param rawValue Raw ADC reading (0-4095)
     * @param timestamp Current timestamp in microseconds
     */
    void processSample(uint8_t padId, uint16_t rawValue, uint32_t timestamp);

    /**
     * @brief Get current state of a pad (for debugging)
     * @param padId Pad ID
     * @return Current trigger state
     */
    TriggerState getState(uint8_t padId) const;

    /**
     * @brief Get baseline value for a pad (for debugging)
     * @param padId Pad ID
     * @return Current baseline ADC value
     */
    uint16_t getBaseline(uint8_t padId) const;

    /**
     * @brief Reset state for a specific pad
     * @param padId Pad ID
     */
    void resetPad(uint8_t padId);

    /**
     * @brief Reset all pads
     */
    void resetAll();

    /**
     * @brief Print current state of all pads (debugging)
     */
    void printState() const;

private:
    PadState padStates[NUM_PADS];  // State for each pad
    QueueHandle_t hitEventQueue;   // Queue to send hit events

    /**
     * @brief Update baseline tracking (exponential moving average)
     * @param padId Pad ID
     * @param rawValue Current raw ADC value
     */
    void updateBaseline(uint8_t padId, uint16_t rawValue);

    /**
     * @brief Convert peak ADC value to MIDI velocity
     * @param peakValue Peak ADC reading
     * @param padId Pad ID (for per-pad calibration)
     * @return MIDI velocity (1-127)
     */
    uint8_t peakToVelocity(uint16_t peakValue, uint8_t padId);

    /**
     * @brief Check if current hit is likely crosstalk
     * Compares timing and velocity with other pads
     * @param currentPad Pad being evaluated
     * @param timestamp Current timestamp
     * @param velocity Calculated velocity of current hit
     * @return true if likely crosstalk (should be rejected)
     */
    bool isCrosstalk(uint8_t currentPad, uint32_t timestamp, uint8_t velocity);

    /**
     * @brief Send hit event to queue
     * @param padId Pad ID
     * @param velocity MIDI velocity
     * @param timestamp Timestamp
     */
    void sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp);
};

// ============================================================
// GLOBAL INSTANCE
// ============================================================

extern TriggerDetector triggerDetector;
