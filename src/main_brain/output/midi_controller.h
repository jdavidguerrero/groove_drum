#ifndef MIDI_CONTROLLER_H
#define MIDI_CONTROLLER_H

#include <Arduino.h>

namespace MIDIController {

constexpr uint8_t  MIDI_CHANNEL      = 10;
constexpr uint32_t NOTE_OFF_DURATION = 50;

struct NoteOffEvent {
    uint8_t  note;
    uint8_t  channel;
    uint32_t offTime;
};

void begin();
void update();

void sendNoteOn(uint8_t note, uint8_t velocity);
void sendNoteOff(uint8_t note);
void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void sendNoteOff(uint8_t channel, uint8_t note);
void sendControlChange(uint8_t control, uint8_t value);
void sendControlChange(uint8_t channel, uint8_t control, uint8_t value);
bool isConnected();

}  // namespace MIDIController

#endif // MIDI_CONTROLLER_H
