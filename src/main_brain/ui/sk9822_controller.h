#ifndef SK9822_CONTROLLER_H
#define SK9822_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>

// ============================================================================
// ENCODER LED CONTROLLER - SK9822/APA102 RING LEDS
// ============================================================================
// Controls 2 encoder rings (12 LEDs each = 24 total)
// SK9822 (APA102) features:
// - High refresh rate (20kHz vs 800Hz for WS2812B)
// - Separate clock line (more reliable than WS2812B timing)
// - Individual brightness control per LED (5-bit global + RGB)

#define NUM_ENCODER_LEDS 24
#define LEDS_PER_RING 12

namespace EncoderLEDController {

// Animation modes for encoder rings
enum AnimationMode {
    ANIM_IDLE_BREATHING,    // Slow breathing effect
    ANIM_VALUE_METER,       // Show value as arc (0-360°)
    ANIM_SPINNING,          // Rotating effect
    ANIM_PULSE,             // Quick pulse when encoder turned
    ANIM_RAINBOW,           // Rainbow cycle
    ANIM_SOLID              // Solid color
};

// Per-encoder state
struct EncoderLEDState {
    AnimationMode mode;
    CRGB baseColor;
    uint8_t brightness;
    float value;            // 0.0-1.0 for meter mode
    uint32_t animationTime;
    bool needsUpdate;
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize SK9822 LEDs
void begin();

// Set animation mode for encoder
void setMode(uint8_t encoderId, AnimationMode mode);

// Set base color
void setColor(uint8_t encoderId, CRGB color);

// Set value for meter mode (0.0-1.0)
void setValue(uint8_t encoderId, float value);

// Trigger pulse animation (when encoder is turned)
void pulse(uint8_t encoderId);

// Update animations (call from main loop at ~60 FPS)
void update();

// Set brightness (0-255)
void setBrightness(uint8_t encoderId, uint8_t brightness);

// Clear all LEDs
void clear();

// Test pattern
void testPattern();

}  // namespace EncoderLEDController

#endif  // SK9822_CONTROLLER_H
