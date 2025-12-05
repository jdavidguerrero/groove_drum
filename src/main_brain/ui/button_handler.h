#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "edrum_config.h"

// ============================================================================
// BUTTON HANDLER - TACTILE BUTTONS WITH ADVANCED DETECTION
// ============================================================================
// Handles 6 tactile buttons with:
// - Debouncing (software)
// - Short press detection
// - Long press detection (>500ms)
// - Double click detection (<300ms between clicks)
// - Hold and repeat (for continuous input)

namespace ButtonHandler {

// Button events
enum ButtonEvent {
    EVENT_NONE,
    EVENT_PRESSED,          // Button just pressed
    EVENT_RELEASED,         // Button just released
    EVENT_CLICK,            // Short press (<500ms)
    EVENT_LONG_PRESS,       // Held >500ms
    EVENT_DOUBLE_CLICK,     // Two clicks within 300ms
    EVENT_HOLD_REPEAT       // Held, repeating every 100ms
};

// Button state
struct ButtonState {
    bool isPressed;
    bool wasPressed;
    uint32_t pressTime;
    uint32_t releaseTime;
    uint32_t lastEventTime;
    uint32_t lastRepeatTime;
    uint8_t clickCount;
    bool longPressTriggered;
    bool repeatTriggered;
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize buttons
void begin();

// Update button states (call from loop frequently)
void update();

// Check if button is currently pressed
bool isPressed(ButtonID buttonId);

// Poll for button events (non-blocking)
ButtonEvent pollEvent(ButtonID buttonId);

// Get button state (for debugging)
const ButtonState& getState(ButtonID buttonId);

// Enable/disable specific button
void setEnabled(ButtonID buttonId, bool enabled);

}  // namespace ButtonHandler

#endif  // BUTTON_HANDLER_H
