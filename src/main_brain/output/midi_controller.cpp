#include "midi_controller.h"

namespace MIDIController {

// ============================================================================
// STATE
// ============================================================================

static bool initialized = false;

// Queue para note offs programados
#define MAX_NOTE_OFFS 8
static NoteOffEvent noteOffQueue[MAX_NOTE_OFFS];
static uint8_t noteOffCount = 0;

// ============================================================================
// HELPER FUNCTIONS - TinyUSB MIDI Packet Construction
// ============================================================================

// Crear un MIDI packet de 4 bytes para TinyUSB
// Formato: [cable_number + code_index_number, MIDI_0, MIDI_1, MIDI_2]
static void sendMIDIPacket(uint8_t status, uint8_t data1, uint8_t data2) {
    if (!initialized) return;

    uint8_t packet[4];

    // Byte 0: Cable Number (upper 4 bits) + Code Index Number (lower 4 bits)
    // Code Index Number para Note On/Off = 0x9 (note on) o 0x8 (note off)
    uint8_t cin = (status & 0xF0) >> 4;  // Extraer el nibble superior del status
    packet[0] = (MIDI_CABLE_NUM << 4) | cin;

    // Bytes 1-3: MIDI message estándar
    packet[1] = status;
    packet[2] = data1;
    packet[3] = data2;

    // Enviar packet via TinyUSB
    tud_midi_packet_write(packet);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    // Initialize USB subsystem
    USB.begin();

    // Wait for USB to be ready
    delay(100);

    initialized = true;
    noteOffCount = 0;

    Serial.println("[MIDI] USB MIDI controller initialized");
    Serial.println("[MIDI] Device: ESP32-S3 E-Drum Controller");
    Serial.println("[MIDI] Connection: Native USB MIDI via TinyUSB");
    Serial.println("[MIDI] Protocol: USB MIDI Class (no adapter needed)");
    Serial.printf("[MIDI] Channel: %d\n", MIDI_CHANNEL + 1);
    Serial.printf("[MIDI] Note off duration: %d ms\n", NOTE_OFF_DURATION);
    Serial.println("[MIDI] Connect USB cable to computer to use");
    Serial.println("[MIDI] The device will appear as 'TinyUSB Device' in your DAW");
}

// ============================================================================
// MIDI OUTPUT
// ============================================================================

void sendNoteOn(uint8_t note, uint8_t velocity) {
    if (!initialized) return;
    if (!tud_midi_mounted()) return;  // Solo enviar si USB está conectado

    // MIDI Note On: status = 0x90 | channel
    uint8_t status = 0x90 | (MIDI_CHANNEL & 0x0F);
    sendMIDIPacket(status, note & 0x7F, velocity & 0x7F);

    // Schedule Note Off
    if (noteOffCount < MAX_NOTE_OFFS) {
        noteOffQueue[noteOffCount].note = note;
        noteOffQueue[noteOffCount].offTime = millis() + NOTE_OFF_DURATION;
        noteOffCount++;
    }

    #ifdef DEBUG_MIDI
    Serial.printf("[MIDI] Note On: %d, Velocity: %d\n", note, velocity);
    #endif
}

void sendNoteOff(uint8_t note) {
    if (!initialized) return;
    if (!tud_midi_mounted()) return;  // Solo enviar si USB está conectado

    // MIDI Note Off: status = 0x80 | channel
    uint8_t status = 0x80 | (MIDI_CHANNEL & 0x0F);
    sendMIDIPacket(status, note & 0x7F, 0);

    #ifdef DEBUG_MIDI
    Serial.printf("[MIDI] Note Off: %d\n", note);
    #endif
}

// ============================================================================
// UPDATE (process scheduled note offs)
// ============================================================================

void update() {
    if (!initialized || noteOffCount == 0) return;

    uint32_t now = millis();

    // Check for notes that need to be turned off
    for (uint8_t i = 0; i < noteOffCount; i++) {
        if (now >= noteOffQueue[i].offTime) {
            // Send note off
            sendNoteOff(noteOffQueue[i].note);

            // Remove from queue (shift array)
            for (uint8_t j = i; j < noteOffCount - 1; j++) {
                noteOffQueue[j] = noteOffQueue[j + 1];
            }
            noteOffCount--;
            i--;  // Recheck this position
        }
    }
}

// ============================================================================
// STATUS
// ============================================================================

bool isConnected() {
    return initialized && tud_midi_mounted();
}

}  // namespace MIDIController
