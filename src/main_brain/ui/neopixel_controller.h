#ifndef NEOPIXEL_CONTROLLER_H
#define NEOPIXEL_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>

// ============================================================================
// NEOPIXEL CONTROLLER - WS2812B PAD LEDS
// ============================================================================
// Controls 4 NeoPixel LEDs (one per drum pad)
// Features:
// - Hit flash animation (instant bright → fade to idle)
// - Idle color with breathing effect
// - Configurable colors per pad from PadConfig
// - Smooth transitions using FastLED library

#define NUM_PAD_LEDS 4

namespace NeoPixelController {

// LED animation states
enum AnimationState {
    STATE_IDLE,
    STATE_HIT_FLASH,
    STATE_FADING
};

// Per-pad LED state
struct PadLEDState {
    CRGB currentColor;
    CRGB targetColor;
    CRGB idleColor;
    AnimationState state;
    uint32_t animationStartTime;
    uint16_t fadeDuration;
    uint8_t brightness;
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize NeoPixels
void begin();

// Set pad LED color immediately (called when hit detected)
void flashPad(uint8_t padId, uint32_t color, uint8_t brightness, uint16_t fadeDuration);

// Set idle color for pad
void setIdleColor(uint8_t padId, uint32_t color, uint8_t brightness);

// Update animations (call from main loop at ~60 FPS)
void update();

// Set all LEDs to a color (for testing)
void setAll(CRGB color);

// Set brightness (0-255)
void setBrightness(uint8_t brightness);

// Turn off all LEDs
void clear();

// Get current pad state (for debugging)
const PadLEDState& getPadState(uint8_t padId);

}  // namespace NeoPixelController

#endif  // NEOPIXEL_CONTROLLER_H
