#ifndef MIDI_CONTROLLER_H
#define MIDI_CONTROLLER_H

#include <Arduino.h>

namespace MIDIController {

// Configuración MIDI
constexpr uint8_t  MIDI_CHANNEL        = 10;  // Canal 10 (estándar para drums)
constexpr uint32_t NOTE_OFF_DURATION   = 50;  // ms

// Estructura para gestionar Note-Offs automáticos
struct NoteOffEvent {
    uint8_t  note;
    uint8_t  channel;
    uint32_t offTime;
};

// Inicialización - debe llamarse en setup() ANTES de Serial.begin()
void begin();

// Envío de notas (canal por defecto o específico)
void sendNoteOn(uint8_t note, uint8_t velocity);
void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);

void sendNoteOff(uint8_t note);
void sendNoteOff(uint8_t channel, uint8_t note);

// Control Change (para hi-hat, expresión, etc.)
void sendControlChange(uint8_t control, uint8_t value);
void sendControlChange(uint8_t channel, uint8_t control, uint8_t value);

// Actualización - debe llamarse en loop() para procesar note-offs
void update();

// Estado de conexión USB
bool isConnected();

}  // namespace MIDIController

#endif
