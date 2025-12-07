#include "midi_controller.h"

#include <edrum_config.h>
#include <tusb.h>
#include "class/midi/midi_device.h"

namespace MIDIController {

// ============================================================================
// STATE
// ============================================================================

static bool initialized = false;

constexpr uint8_t MAX_NOTE_OFFS = 16;
static NoteOffEvent noteOffQueue[MAX_NOTE_OFFS];
static uint8_t noteOffCount = 0;

// ============================================================================
// HELPERS
// ============================================================================

static uint8_t clampChannel(uint8_t ch) {
    if (ch < 1) return 1;
    if (ch > 16) return 16;
    return ch;
}

static bool sendMidiPacket(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2) {
    if (!initialized || !tud_midi_mounted()) return false;

    uint8_t packet[4] = {
        static_cast<uint8_t>((cin << 4) | 0),  // Cable 0 + CIN
        status,
        static_cast<uint8_t>(data1 & 0x7F),
        static_cast<uint8_t>(data2 & 0x7F)
    };

    return tud_midi_packet_write(packet);
}

static void logRxPacket(const uint8_t packet[4]) {
#ifdef DEBUG_MIDI_MESSAGES
    Serial.printf("[MIDI RX] %02X %02X %02X %02X\n",
                  packet[0], packet[1], packet[2], packet[3]);
#else
    (void)packet;
#endif
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin() {
    if (initialized) return;

    // Espera breve a que TinyUSB esté listo
    uint32_t start = millis();
    while (!tud_inited() && (millis() - start < 2000)) {
        delay(10);
    }

    initialized = true;
    noteOffCount = 0;

    Serial.println("[MIDI] USB MIDI listo (TinyUSB nativo)");
    Serial.printf("[MIDI] Canal por defecto: %u\n", MIDI_CHANNEL);
}

// ============================================================================
// NOTE ON / NOTE OFF
// ============================================================================

void sendNoteOn(uint8_t note, uint8_t velocity) {
    sendNoteOn(MIDI_CHANNEL, note, velocity);
}

void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!initialized) return;

    uint8_t ch = clampChannel(channel);
    uint8_t status = 0x90 | (ch - 1);

    sendMidiPacket(0x09, status, note, velocity);

    if (noteOffCount < MAX_NOTE_OFFS) {
        noteOffQueue[noteOffCount] = {note, ch, millis() + NOTE_OFF_DURATION};
        noteOffCount++;
    }

#ifdef DEBUG_MIDI_MESSAGES
    Serial.printf("[MIDI TX] NoteOn ch=%u note=%u vel=%u\n", ch, note, velocity);
#endif
}

void sendNoteOff(uint8_t note) {
    sendNoteOff(MIDI_CHANNEL, note);
}

void sendNoteOff(uint8_t channel, uint8_t note) {
    if (!initialized) return;

    uint8_t ch = clampChannel(channel);
    uint8_t status = 0x80 | (ch - 1);

    sendMidiPacket(0x08, status, note, 0);

#ifdef DEBUG_MIDI_MESSAGES
    Serial.printf("[MIDI TX] NoteOff ch=%u note=%u\n", ch, note);
#endif
}

// ============================================================================
// CONTROL CHANGE
// ============================================================================

void sendControlChange(uint8_t control, uint8_t value) {
    sendControlChange(MIDI_CHANNEL, control, value);
}

void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    if (!initialized) return;

    uint8_t ch = clampChannel(channel);
    uint8_t status = 0xB0 | (ch - 1);

    sendMidiPacket(0x0B, status, control, value);

#ifdef DEBUG_MIDI_MESSAGES
    Serial.printf("[MIDI TX] CC ch=%u ctrl=%u val=%u\n", ch, control, value);
#endif
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void update() {
    if (!initialized) return;

    // Procesar note-offs programados
    if (noteOffCount > 0) {
        uint32_t now = millis();
        for (uint8_t i = 0; i < noteOffCount; ) {
            if (now >= noteOffQueue[i].offTime) {
                sendNoteOff(noteOffQueue[i].channel, noteOffQueue[i].note);
                for (uint8_t j = i; j < noteOffCount - 1; j++) {
                    noteOffQueue[j] = noteOffQueue[j + 1];
                }
                noteOffCount--;
            } else {
                ++i;
            }
        }
    }

    // Leer MIDI entrante (solo log)
    uint8_t packet[4];
    while (tud_midi_available() && tud_midi_packet_read(packet)) {
        logRxPacket(packet);
    }
}

// ============================================================================
// STATUS
// ============================================================================

bool isConnected() {
    return initialized && tud_midi_mounted();
}

}  // namespace MIDIController
