#include "sk9822_controller.h"
#include "edrum_config.h"

namespace EncoderLEDController {

// ============================================================================
// STATE
// ============================================================================

static CRGB leds[NUM_ENCODER_LEDS];
static EncoderLEDState encoderStates[NUM_ENCODERS];
static uint32_t lastUpdateTime = 0;
static const uint32_t UPDATE_INTERVAL_MS = 16;  // ~60 FPS

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    // Initialize FastLED for SK9822/APA102
    FastLED.addLeds<SK9822, LED_ENC_DATA_PIN, LED_ENC_CLK_PIN, BGR>(leds, NUM_ENCODER_LEDS);
    FastLED.setBrightness(128);  // Default 50%
    FastLED.clear();
    FastLED.show();

    // Initialize encoder states
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        encoderStates[i].mode = ANIM_IDLE_BREATHING;
        encoderStates[i].baseColor = CRGB::Cyan;
        encoderStates[i].brightness = 128;
        encoderStates[i].value = 0.5f;
        encoderStates[i].animationTime = 0;
        encoderStates[i].needsUpdate = true;
    }

    Serial.println("[SK9822] Controller initialized");
    Serial.printf("  Encoder LEDs: %d (%d rings x %d LEDs)\n",
                  NUM_ENCODER_LEDS, NUM_ENCODERS, LEDS_PER_RING);
    Serial.printf("  Data pin: %d, Clock pin: %d\n", LED_ENC_DATA_PIN, LED_ENC_CLK_PIN);
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void setMode(uint8_t encoderId, AnimationMode mode) {
    if (encoderId >= NUM_ENCODERS) return;
    encoderStates[encoderId].mode = mode;
    encoderStates[encoderId].needsUpdate = true;
}

void setColor(uint8_t encoderId, CRGB color) {
    if (encoderId >= NUM_ENCODERS) return;
    encoderStates[encoderId].baseColor = color;
    encoderStates[encoderId].needsUpdate = true;
}

void setValue(uint8_t encoderId, float value) {
    if (encoderId >= NUM_ENCODERS) return;
    encoderStates[encoderId].value = constrain(value, 0.0f, 1.0f);
    encoderStates[encoderId].needsUpdate = true;
}

void setBrightness(uint8_t encoderId, uint8_t brightness) {
    if (encoderId >= NUM_ENCODERS) return;
    encoderStates[encoderId].brightness = brightness;
    encoderStates[encoderId].needsUpdate = true;
}

void pulse(uint8_t encoderId) {
    if (encoderId >= NUM_ENCODERS) return;
    // Quick mode switch for pulse effect
    EncoderLEDState& enc = encoderStates[encoderId];
    AnimationMode prevMode = enc.mode;
    enc.mode = ANIM_PULSE;
    enc.animationTime = millis();
    enc.needsUpdate = true;

    // Reset to previous mode after 200ms (handled in update())
}

// ============================================================================
// UPDATE ANIMATIONS
// ============================================================================

void update() {
    uint32_t now = millis();

    // Throttle to ~60 FPS
    if (now - lastUpdateTime < UPDATE_INTERVAL_MS) {
        return;
    }
    lastUpdateTime = now;

    for (uint8_t encId = 0; encId < NUM_ENCODERS; encId++) {
        EncoderLEDState& enc = encoderStates[encId];
        uint16_t startLED = encId * LEDS_PER_RING;

        switch (enc.mode) {
            case ANIM_IDLE_BREATHING: {
                // Slow breathing (2 second period)
                float breath = (sin((now / 2000.0f) * TWO_PI) + 1.0f) / 2.0f;  // 0.0-1.0
                uint8_t brightness = (uint8_t)(breath * enc.brightness);

                for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                    leds[startLED + i] = enc.baseColor;
                    leds[startLED + i].fadeLightBy(255 - brightness);
                }
                enc.needsUpdate = true;
                break;
            }

            case ANIM_VALUE_METER: {
                // Show value as arc (like volume meter)
                uint8_t numLit = (uint8_t)(enc.value * LEDS_PER_RING);

                for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                    if (i < numLit) {
                        // Gradient from green → yellow → red
                        float ledValue = (float)i / (float)LEDS_PER_RING;
                        CRGB color = blend(CRGB::Green, CRGB::Red, ledValue * 255);
                        leds[startLED + i] = color;
                        leds[startLED + i].fadeLightBy(255 - enc.brightness);
                    } else {
                        leds[startLED + i] = CRGB::Black;
                    }
                }
                enc.needsUpdate = true;
                break;
            }

            case ANIM_SPINNING: {
                // Rotating comet effect
                uint8_t pos = (now / 50) % LEDS_PER_RING;

                for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                    uint8_t distance = abs((int)i - (int)pos);
                    if (distance > LEDS_PER_RING / 2) {
                        distance = LEDS_PER_RING - distance;
                    }

                    uint8_t brightness = 255 - (distance * 60);
                    leds[startLED + i] = enc.baseColor;
                    leds[startLED + i].fadeLightBy(255 - brightness);
                }
                enc.needsUpdate = true;
                break;
            }

            case ANIM_PULSE: {
                // Quick pulse when encoder turned
                uint32_t elapsed = now - enc.animationTime;
                if (elapsed > 200) {
                    // Return to breathing
                    enc.mode = ANIM_IDLE_BREATHING;
                } else {
                    float progress = (float)elapsed / 200.0f;
                    uint8_t brightness = (uint8_t)((1.0f - progress) * enc.brightness);

                    for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                        leds[startLED + i] = CRGB::White;
                        leds[startLED + i].fadeLightBy(255 - brightness);
                    }
                }
                enc.needsUpdate = true;
                break;
            }

            case ANIM_RAINBOW: {
                // Rainbow cycle
                uint8_t hue = (now / 20) % 256;

                for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                    leds[startLED + i] = CHSV(hue + (i * 256 / LEDS_PER_RING), 255, enc.brightness);
                }
                enc.needsUpdate = true;
                break;
            }

            case ANIM_SOLID: {
                // Solid color
                for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
                    leds[startLED + i] = enc.baseColor;
                    leds[startLED + i].fadeLightBy(255 - enc.brightness);
                }
                if (enc.needsUpdate) {
                    enc.needsUpdate = false;
                }
                break;
            }
        }
    }

    FastLED.show();
}

// ============================================================================
// UTILITY
// ============================================================================

void clear() {
    FastLED.clear();
    FastLED.show();
}

void testPattern() {
    Serial.println("[SK9822] Running test pattern...");

    // Test 1: All white
    fill_solid(leds, NUM_ENCODER_LEDS, CRGB::White);
    FastLED.show();
    delay(1000);

    // Test 2: Rainbow
    for (uint8_t i = 0; i < NUM_ENCODER_LEDS; i++) {
        leds[i] = CHSV(i * 256 / NUM_ENCODER_LEDS, 255, 255);
    }
    FastLED.show();
    delay(1000);

    // Test 3: Individual encoder rings
    for (uint8_t enc = 0; enc < NUM_ENCODERS; enc++) {
        FastLED.clear();
        for (uint8_t i = 0; i < LEDS_PER_RING; i++) {
            leds[enc * LEDS_PER_RING + i] = CRGB::Cyan;
        }
        FastLED.show();
        delay(500);
    }

    FastLED.clear();
    FastLED.show();
    Serial.println("[SK9822] Test complete");
}

}  // namespace EncoderLEDController
