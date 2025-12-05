#include "neopixel_controller.h"
#include "edrum_config.h"

namespace NeoPixelController {

// ============================================================================
// STATE
// ============================================================================

static CRGB leds[NUM_PAD_LEDS];
static PadLEDState padStates[NUM_PAD_LEDS];
static uint32_t lastUpdateTime = 0;
static const uint32_t UPDATE_INTERVAL_MS = 16;  // ~60 FPS

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    // Initialize FastLED for WS2812B
    FastLED.addLeds<WS2812B, LED_PADS_PIN, GRB>(leds, NUM_PAD_LEDS);
    FastLED.setBrightness(255);
    FastLED.clear();
    FastLED.show();

    // Initialize pad states
    for (uint8_t i = 0; i < NUM_PAD_LEDS; i++) {
        padStates[i].currentColor = CRGB::Black;
        padStates[i].targetColor = CRGB::Black;
        padStates[i].idleColor = CRGB(30, 30, 30);  // Dim white default
        padStates[i].state = STATE_IDLE;
        padStates[i].brightness = 255;
        padStates[i].fadeDuration = 200;
    }

    Serial.println("[NeoPixel] Controller initialized");
    Serial.printf("  Pad LEDs: %d\n", NUM_PAD_LEDS);
    Serial.printf("  Pin: %d\n", LED_PADS_PIN);
}

// ============================================================================
// FLASH PAD (called when hit detected)
// ============================================================================

void flashPad(uint8_t padId, uint32_t color, uint8_t brightness, uint16_t fadeDuration) {
    if (padId >= NUM_PAD_LEDS) return;

    PadLEDState& pad = padStates[padId];

    // Convert uint32_t (0xRRGGBB) to CRGB
    pad.currentColor = CRGB(
        (color >> 16) & 0xFF,  // Red
        (color >> 8) & 0xFF,   // Green
        color & 0xFF           // Blue
    );

    pad.targetColor = pad.idleColor;
    pad.brightness = brightness;
    pad.fadeDuration = fadeDuration;
    pad.state = STATE_HIT_FLASH;
    pad.animationStartTime = millis();

    // Immediately show hit color
    leds[padId] = pad.currentColor;
    leds[padId].fadeLightBy(255 - pad.brightness);
    FastLED.show();
}

// ============================================================================
// SET IDLE COLOR
// ============================================================================

void setIdleColor(uint8_t padId, uint32_t color, uint8_t brightness) {
    if (padId >= NUM_PAD_LEDS) return;

    PadLEDState& pad = padStates[padId];

    pad.idleColor = CRGB(
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF
    );

    pad.brightness = brightness;

    // If idle, update immediately
    if (pad.state == STATE_IDLE) {
        pad.currentColor = pad.idleColor;
        leds[padId] = pad.currentColor;
        leds[padId].fadeLightBy(255 - pad.brightness);
    }
}

// ============================================================================
// UPDATE ANIMATIONS (call from loop at 60 FPS)
// ============================================================================

void update() {
    uint32_t now = millis();

    // Throttle updates to ~60 FPS
    if (now - lastUpdateTime < UPDATE_INTERVAL_MS) {
        return;
    }
    lastUpdateTime = now;

    bool needsUpdate = false;

    for (uint8_t i = 0; i < NUM_PAD_LEDS; i++) {
        PadLEDState& pad = padStates[i];

        switch (pad.state) {
            case STATE_HIT_FLASH: {
                // Flash is instant, transition to fading
                pad.state = STATE_FADING;
                needsUpdate = true;
                break;
            }

            case STATE_FADING: {
                uint32_t elapsed = now - pad.animationStartTime;
                float progress = (float)elapsed / (float)pad.fadeDuration;

                if (progress >= 1.0f) {
                    // Fade complete, return to idle
                    pad.currentColor = pad.idleColor;
                    pad.state = STATE_IDLE;
                    needsUpdate = true;
                } else {
                    // Interpolate from current to idle color
                    pad.currentColor = blend(pad.currentColor, pad.targetColor, progress * 255);
                    needsUpdate = true;
                }
                break;
            }

            case STATE_IDLE: {
                // Optional: Breathing effect
                // float breath = (sin(now / 1000.0f) + 1.0f) / 2.0f;  // 0.0-1.0
                // pad.currentColor = pad.idleColor;
                // pad.currentColor.fadeLightBy(255 - (uint8_t)(breath * pad.brightness));
                // needsUpdate = true;
                break;
            }
        }

        // Update LED
        leds[i] = pad.currentColor;
        leds[i].fadeLightBy(255 - pad.brightness);
    }

    if (needsUpdate) {
        FastLED.show();
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void setAll(CRGB color) {
    fill_solid(leds, NUM_PAD_LEDS, color);
    FastLED.show();
}

void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void clear() {
    FastLED.clear();
    FastLED.show();

    for (uint8_t i = 0; i < NUM_PAD_LEDS; i++) {
        padStates[i].state = STATE_IDLE;
        padStates[i].currentColor = CRGB::Black;
    }
}

const PadLEDState& getPadState(uint8_t padId) {
    static PadLEDState dummy;
    if (padId >= NUM_PAD_LEDS) return dummy;
    return padStates[padId];
}

}  // namespace NeoPixelController
