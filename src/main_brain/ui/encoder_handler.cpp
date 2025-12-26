#include "encoder_handler.h"
#include "edrum_config.h"

namespace EncoderHandler {

// ============================================================================
// STATE
// ============================================================================

static EncoderState encoders[NUM_ENCODERS];

// Pin definitions per encoder (int8_t for SW to allow -1 for disabled)
struct EncoderPins {
    uint8_t pinA;
    uint8_t pinB;
    int8_t pinSW;  // -1 means disabled
};

static const EncoderPins encoderPins[NUM_ENCODERS] = {
    {ENC_L_A_PIN, ENC_L_B_PIN, ENC_L_SW_PIN},  // Left encoder
    {ENC_R_A_PIN, ENC_R_B_PIN, ENC_R_SW_PIN}   // Right encoder
};

// Debounce
static const uint32_t DEBOUNCE_MS = 5;
static const uint32_t LONG_PRESS_MS = 500;

// Quadrature lookup table for state machine
// [oldAB][newAB] → direction (-1, 0, +1)
static const int8_t quadratureTable[4][4] = {
    // NEW: 00  01  10  11
    {  0, -1, +1,  0 },  // OLD: 00
    { +1,  0,  0, -1 },  // OLD: 01
    { -1,  0,  0, +1 },  // OLD: 10
    {  0, +1, -1,  0 }   // OLD: 11
};

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    // Initialize encoder states
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        encoders[i].position = 0;
        encoders[i].delta = 0;
        encoders[i].switchPressed = false;
        encoders[i].switchPressTime = 0;
        encoders[i].lastRotationTime = 0;
        encoders[i].longPressTriggered = false;

        // Configure pins
        pinMode(encoderPins[i].pinA, INPUT_PULLUP);
        pinMode(encoderPins[i].pinB, INPUT_PULLUP);

        // Only configure switch if not disabled
        if (encoderPins[i].pinSW >= 0) {
            pinMode(encoderPins[i].pinSW, INPUT_PULLUP);
        }

        // Read initial state
        bool stateA = digitalRead(encoderPins[i].pinA);
        bool stateB = digitalRead(encoderPins[i].pinB);
        encoders[i].lastAB = (stateA << 1) | stateB;
    }

    Serial.println("[Encoder] Handler initialized");
    Serial.printf("  Encoders: %d\n", NUM_ENCODERS);
}

// ============================================================================
// UPDATE (call frequently from loop or timer)
// ============================================================================

void update() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        EncoderState& enc = encoders[i];
        const EncoderPins& pins = encoderPins[i];

        // 1. READ ROTATION (Quadrature decoding)
        bool stateA = digitalRead(pins.pinA);
        bool stateB = digitalRead(pins.pinB);
        uint8_t newAB = (stateA << 1) | stateB;

        if (newAB != enc.lastAB) {
            // State changed, lookup direction
            int8_t direction = quadratureTable[enc.lastAB][newAB];

            if (direction != 0) {
                // Valid transition
                enc.position += direction;
                enc.delta += direction;
                enc.lastRotationTime = now;

                // Optional: Acceleration based on speed
                // uint32_t timeSinceLastRotation = now - enc.lastRotationTime;
                // if (timeSinceLastRotation < 20) {  // Fast rotation
                //     enc.position += direction;  // Double step
                //     enc.delta += direction;
                // }
            }

            enc.lastAB = newAB;
        }

        // 2. READ SWITCH with debounce (skip if disabled)
        if (pins.pinSW < 0) continue;

        bool switchReading = !digitalRead(pins.pinSW);  // Active LOW

        if (switchReading && !enc.switchPressed) {
            // Switch just pressed
            enc.switchPressed = true;
            enc.switchPressTime = now;
            enc.longPressTriggered = false;

        } else if (!switchReading && enc.switchPressed) {
            // Switch just released
            enc.switchPressed = false;

        } else if (enc.switchPressed && !enc.longPressTriggered) {
            // Check for long press
            if (now - enc.switchPressTime > LONG_PRESS_MS) {
                enc.longPressTriggered = true;
            }
        }
    }
}

// ============================================================================
// GETTERS
// ============================================================================

int32_t getPosition(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return 0;
    return encoders[encoderId].position;
}

int32_t getDelta(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return 0;

    int32_t delta = encoders[encoderId].delta;
    encoders[encoderId].delta = 0;  // Reset after reading
    return delta;
}

void resetPosition(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return;
    encoders[encoderId].position = 0;
    encoders[encoderId].delta = 0;
}

void setPosition(uint8_t encoderId, int32_t position) {
    if (encoderId >= NUM_ENCODERS) return;
    encoders[encoderId].position = position;
}

bool isSwitchPressed(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return false;
    return encoders[encoderId].switchPressed;
}

// ============================================================================
// EVENT POLLING
// ============================================================================

EncoderEvent pollEvent(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return EVENT_NONE;

    EncoderState& enc = encoders[encoderId];

    // Check rotation first
    if (enc.delta != 0) {
        int32_t delta = enc.delta;
        enc.delta = 0;

        if (delta > 0) {
            return EVENT_ROTATED_CW;
        } else {
            return EVENT_ROTATED_CCW;
        }
    }

    // Check switch events
    static bool lastSwitchState[NUM_ENCODERS] = {false};

    if (enc.switchPressed && !lastSwitchState[encoderId]) {
        lastSwitchState[encoderId] = true;
        return EVENT_SWITCH_PRESSED;

    } else if (!enc.switchPressed && lastSwitchState[encoderId]) {
        lastSwitchState[encoderId] = false;
        return EVENT_SWITCH_RELEASED;

    } else if (enc.longPressTriggered) {
        enc.longPressTriggered = false;  // Clear flag
        return EVENT_SWITCH_LONG_PRESS;
    }

    return EVENT_NONE;
}

const EncoderState& getState(uint8_t encoderId) {
    static EncoderState dummy;
    if (encoderId >= NUM_ENCODERS) return dummy;
    return encoders[encoderId];
}

}  // namespace EncoderHandler
