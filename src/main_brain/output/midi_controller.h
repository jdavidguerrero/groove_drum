#ifndef MIDI_CONTROLLER_H
#define MIDI_CONTROLLER_H

#include <Arduino.h>

namespace MIDIController {

// ============================================================
// CONSTANTES DE CONFIGURACIÓN
// ============================================================
// Canal por defecto (10 es el estándar GM para percusión)
constexpr uint8_t  MIDI_CHANNEL        = 10; 

// Duración automática para enviar el NoteOff si no se maneja manual
constexpr uint32_t NOTE_OFF_DURATION   = 50;  // ms

// ============================================================
// ESTRUCTURAS DE DATOS
// ============================================================
struct NoteOffEvent {
    uint8_t  note;
    uint8_t  channel;
    uint32_t offTime;
};

// ============================================================
// API PÚBLICA
// ============================================================

/**
 * @brief Inicializa el stack USB nativo (TinyUSB) y el protocolo MIDI.
 * Debe llamarse en setup() antes de cualquier Serial.print
 */
void begin();

/**
 * @brief Procesa tareas de fondo (lectura de USB y gestión de NoteOffs).
 * DEBE llamarse en cada iteración del loop().
 */
void update();

// --- Envío de Notas ---

// Usa el canal por defecto (10)
void sendNoteOn(uint8_t note, uint8_t velocity);
void sendNoteOff(uint8_t note);

// Usa un canal específico (1-16)
void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void sendNoteOff(uint8_t channel, uint8_t note);

// --- Control Change (CC) ---
// Útil para pedal de Hi-Hat (CC 4) o volumen (CC 7)
void sendControlChange(uint8_t control, uint8_t value);
void sendControlChange(uint8_t channel, uint8_t control, uint8_t value);

/**
 * @brief Verifica si el cable USB está conectado y montado en el OS.
 * @return true si el PC ha reconocido el dispositivo.
 */
bool isConnected();

}  // namespace MIDIController

#endif // MIDI_CONTROLLER_H
