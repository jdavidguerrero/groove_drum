#include "ring_led_controller.h"

#include <FastLED.h>
#include <edrum_config.h>

namespace display {
namespace {
CRGB leds[NUM_LEDS_RING];
constexpr CRGB kPadColors[4] = {
    CRGB(0, 255, 255),   // Kick
    CRGB(255, 50, 150),  // Snare
    CRGB(255, 255, 0),   // HiHat
    CRGB(0, 255, 100)    // Tom
};
constexpr uint8_t kPadSegmentWidth = 4;
constexpr uint8_t kPadStartIndex[4] = {0, 3, 6, 9};
uint32_t lastFadeMs = 0;
}  // namespace

void RingLEDController::begin() {
    FastLED.addLeds<NEOPIXEL, LED_RING_PIN>(leds, NUM_LEDS_RING);
    FastLED.setBrightness(96);
    FastLED.clear(true);
    applyIdleGlow();
    FastLED.show();
}

void RingLEDController::pulsePad(uint8_t padId, uint8_t velocity) {
    if (padId >= 4) {
        return;
    }

    const uint8_t start = kPadStartIndex[padId % 4];
    const uint8_t level = constrain(map(velocity, 1, 127, 80, 255), 0, 255);
    const CRGB baseColor = kPadColors[padId];

    for (uint8_t i = 0; i < kPadSegmentWidth; ++i) {
        uint8_t idx = (start + i) % NUM_LEDS_RING;
        leds[idx] = baseColor;
        leds[idx].nscale8_video(level);
    }
    FastLED.show();
}

void RingLEDController::update() {
    const uint32_t now = millis();
    if (now - lastFadeMs < 20) {
        return;
    }
    lastFadeMs = now;

    fadeToBlackBy(leds, NUM_LEDS_RING, 8);
    applyIdleGlow();
    FastLED.show();
}

void RingLEDController::applyIdleGlow() {
    // Keep a subtle ambient ring when no hits are happening
    for (uint8_t i = 0; i < NUM_LEDS_RING; ++i) {
        if (leds[i].getAverageLight() < 6) {
            leds[i] = CRGB(2, 2, 4);
        }
    }
}

}  // namespace display
