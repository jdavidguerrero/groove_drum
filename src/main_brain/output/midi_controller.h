#ifndef MIDI_CONTROLLER_H
#define MIDI_CONTROLLER_H

#include <Arduino.h>
#include <USB.h>

// TinyUSB MIDI support
extern "C" {
    #include "class/midi/midi_device.h"
}

// ============================================================================
// MIDI CONTROLLER - USB MIDI OUTPUT
// ============================================================================
// Handles MIDI note output for drum pads via USB MIDI (native TinyUSB)
// - Note On/Off messages
// - Velocity mapping
// - Automatic note off scheduling
//
// Hardware: Conectar ESP32-S3 por USB (aparece como dispositivo MIDI)
// Basado en TinyUSB MIDI Device API

namespace MIDIController {

// MIDI Configuration
#define MIDI_CHANNEL 0        // Canal 1 (0-indexed)
#define NOTE_OFF_DURATION 50  // Duración de la nota en ms
#define MIDI_CABLE_NUM 0      // Cable virtual (siempre 0 para un solo puerto)

// Note Off event structure
struct NoteOffEvent {
    uint8_t note;
    uint32_t offTime;  // millis() cuando debe apagarse
};

// ============================================================================
// INTERFACE
// ============================================================================

// Initialize MIDI controller
void begin();

// Send MIDI Note On
void sendNoteOn(uint8_t note, uint8_t velocity);

// Send MIDI Note Off
void sendNoteOff(uint8_t note);

// Process scheduled note offs (call from loop)
void update();

// Get MIDI status for debugging
bool isConnected();

}  // namespace MIDIController

#endif  // MIDI_CONTROLLER_H
