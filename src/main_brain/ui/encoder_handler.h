#ifndef ENCODER_HANDLER_H
#define ENCODER_HANDLER_H

#include <Arduino.h>

// ============================================================================
// ENCODER HANDLER - ROTARY ENCODERS WITH SWITCH
// ============================================================================
// Handles 2 rotary encoders (ALPS EC11 or similar)
// Features:
// - Quadrature decoding for rotation direction
// - Detent detection (steps)
// - Switch press detection with debounce
// - Long press detection
// - Acceleration (faster turn = bigger steps)

#define NUM_ENCODERS 2

namespace EncoderHandler {

// Encoder events
enum EncoderEvent {
    EVENT_NONE,
    EVENT_ROTATED_CW,       // Clockwise rotation (1 detent)
    EVENT_ROTATED_CCW,      // Counter-clockwise rotation
    EVENT_SWITCH_PRESSED,   // Switch pressed (short)
    EVENT_SWITCH_RELEASED,  // Switch released
    EVENT_SWITCH_LONG_PRESS // Switch held >500ms
};

// Encoder state
struct EncoderState {
    int32_t position;           // Accumulated position (steps)
    int32_t delta;              // Change since last read
    bool switchPressed;
    uint32_t switchPressTime;
    uint32_t lastRotationTime;
    uint8_t lastAB;             // Last A/B pin state
    bool longPressTriggered;
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize encoders
void begin();

// Update encoder states (call from loop or timer interrupt)
void update();

// Get encoder position (accumulated steps)
int32_t getPosition(uint8_t encoderId);

// Get delta since last read (and reset delta)
int32_t getDelta(uint8_t encoderId);

// Reset position to zero
void resetPosition(uint8_t encoderId);

// Set position to specific value
void setPosition(uint8_t encoderId, int32_t position);

// Check if switch is currently pressed
bool isSwitchPressed(uint8_t encoderId);

// Poll for events (non-blocking)
EncoderEvent pollEvent(uint8_t encoderId);

// Get encoder state (for debugging)
const EncoderState& getState(uint8_t encoderId);

}  // namespace EncoderHandler

#endif  // ENCODER_HANDLER_H
