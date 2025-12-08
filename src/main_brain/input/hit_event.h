#ifndef HIT_EVENT_H
#define HIT_EVENT_H

#include <Arduino.h>

struct HitEvent {
    uint8_t padId;
    uint8_t velocity;
    uint32_t timestamp;
    uint16_t peakValue;

    HitEvent() : padId(0), velocity(0), timestamp(0), peakValue(0) {}
    HitEvent(uint8_t id, uint8_t vel, uint32_t time, uint16_t peak = 0)
        : padId(id), velocity(vel), timestamp(time), peakValue(peak) {}
};

#endif // HIT_EVENT_H
