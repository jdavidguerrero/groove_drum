#include "button_handler.h"

namespace ButtonHandler {

// ============================================================================
// STATE
// ============================================================================

static ButtonState buttons[NUM_BUTTONS];
static bool buttonsEnabled[NUM_BUTTONS];

// Pin mapping (int8_t to allow -1 for disabled pins)
static const int8_t buttonPins[NUM_BUTTONS] = {
    BTN_KIT_PIN,      // May be -1 if disabled
    BTN_EDIT_PIN,
    BTN_MENU_PIN,
    BTN_CLICK_PIN,
    BTN_FX_PIN,
    BTN_SHIFT_PIN
};

// Timing constants
static const uint32_t DEBOUNCE_MS = 20;
static const uint32_t LONG_PRESS_MS = 500;
static const uint32_t DOUBLE_CLICK_MS = 300;
static const uint32_t REPEAT_INITIAL_MS = 500;
static const uint32_t REPEAT_RATE_MS = 100;

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    // Initialize button states
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].isPressed = false;
        buttons[i].wasPressed = false;
        buttons[i].pressTime = 0;
        buttons[i].releaseTime = 0;
        buttons[i].lastEventTime = 0;
        buttons[i].lastRepeatTime = 0;
        buttons[i].clickCount = 0;
        buttons[i].longPressTriggered = false;
        buttons[i].repeatTriggered = false;

        // Skip disabled pins (marked as -1)
        if (buttonPins[i] < 0) {
            buttonsEnabled[i] = false;
            continue;
        }

        buttonsEnabled[i] = true;

        // Configure pins (active LOW with internal pullup)
        pinMode(buttonPins[i], INPUT_PULLUP);
    }

    Serial.println("[Button] Handler initialized");
    Serial.printf("  Buttons: %d\n", NUM_BUTTONS);
}

// ============================================================================
// UPDATE (call frequently from loop)
// ============================================================================

void update() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        if (!buttonsEnabled[i]) continue;

        ButtonState& btn = buttons[i];

        // Read pin (active LOW)
        bool reading = !digitalRead(buttonPins[i]);

        // Debounce check
        if (reading != btn.isPressed) {
            uint32_t timeSinceLastChange = now - btn.lastEventTime;

            if (timeSinceLastChange > DEBOUNCE_MS) {
                // Valid state change
                btn.wasPressed = btn.isPressed;
                btn.isPressed = reading;
                btn.lastEventTime = now;

                if (btn.isPressed) {
                    // Just pressed
                    btn.pressTime = now;
                    btn.longPressTriggered = false;
                    btn.repeatTriggered = false;

                } else {
                    // Just released
                    btn.releaseTime = now;

                    // Check for double click
                    uint32_t pressDuration = btn.releaseTime - btn.pressTime;
                    if (pressDuration < LONG_PRESS_MS) {
                        // Short press
                        uint32_t timeSinceLastRelease = now - buttons[i].releaseTime;
                        if (btn.clickCount > 0 && timeSinceLastRelease < DOUBLE_CLICK_MS) {
                            btn.clickCount++;
                        } else {
                            btn.clickCount = 1;
                        }
                    }
                }
            }
        }

        // Check for long press
        if (btn.isPressed && !btn.longPressTriggered) {
            uint32_t pressDuration = now - btn.pressTime;
            if (pressDuration >= LONG_PRESS_MS) {
                btn.longPressTriggered = true;
                btn.lastRepeatTime = now;
            }
        }

        // Check for hold repeat
        if (btn.isPressed && btn.longPressTriggered && !btn.repeatTriggered) {
            uint32_t timeSinceRepeat = now - btn.lastRepeatTime;
            if (timeSinceRepeat >= REPEAT_RATE_MS) {
                btn.repeatTriggered = true;
                btn.lastRepeatTime = now;
            }
        }

        // Reset click count if timeout
        if (btn.clickCount > 0) {
            uint32_t timeSinceRelease = now - btn.releaseTime;
            if (timeSinceRelease > DOUBLE_CLICK_MS) {
                btn.clickCount = 0;
            }
        }
    }
}

// ============================================================================
// GETTERS
// ============================================================================

bool isPressed(ButtonID buttonId) {
    if (buttonId >= NUM_BUTTONS) return false;
    return buttons[buttonId].isPressed;
}

const ButtonState& getState(ButtonID buttonId) {
    static ButtonState dummy;
    if (buttonId >= NUM_BUTTONS) return dummy;
    return buttons[buttonId];
}

void setEnabled(ButtonID buttonId, bool enabled) {
    if (buttonId >= NUM_BUTTONS) return;
    buttonsEnabled[buttonId] = enabled;
}

// ============================================================================
// EVENT POLLING
// ============================================================================

ButtonEvent pollEvent(ButtonID buttonId) {
    if (buttonId >= NUM_BUTTONS) return EVENT_NONE;

    ButtonState& btn = buttons[buttonId];

    // Priority order: Double click > Long press > Click > Repeat

    // Check for double click
    if (btn.clickCount >= 2) {
        btn.clickCount = 0;
        return EVENT_DOUBLE_CLICK;
    }

    // Check for long press (one-shot)
    if (btn.longPressTriggered && !btn.repeatTriggered) {
        return EVENT_LONG_PRESS;
    }

    // Check for hold repeat
    if (btn.repeatTriggered) {
        btn.repeatTriggered = false;
        return EVENT_HOLD_REPEAT;
    }

    // Check for press/release transitions
    if (btn.isPressed && !btn.wasPressed) {
        btn.wasPressed = true;
        return EVENT_PRESSED;
    }

    if (!btn.isPressed && btn.wasPressed) {
        btn.wasPressed = false;

        // Check if it was a short press (click)
        uint32_t pressDuration = btn.releaseTime - btn.pressTime;
        if (pressDuration < LONG_PRESS_MS) {
            return EVENT_CLICK;
        }

        return EVENT_RELEASED;
    }

    return EVENT_NONE;
}

}  // namespace ButtonHandler
