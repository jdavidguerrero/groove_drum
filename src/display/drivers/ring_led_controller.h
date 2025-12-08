#pragma once

#include <Arduino.h>

namespace display {

class RingLEDController {
public:
    static void begin();
    static void pulsePad(uint8_t padId, uint8_t velocity);
    static void update();

private:
    static void applyIdleGlow();
};

}  // namespace display
