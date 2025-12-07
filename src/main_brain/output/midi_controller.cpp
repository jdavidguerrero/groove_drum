/**
 * @file midi_controller.cpp
 * @brief USB MIDI Controller for ESP32-S3 (USB nativo + TinyUSB)
 *
 * Implementa MIDI sobre el stack USB nativo del core Arduino-ESP32
 * siguiendo el ejemplo de https://github.com/esp32beans/ESP32USBMIDI.
 */

#include "midi_controller.h"
#include <Arduino.h>
#include <USB.h>
#include "esp32-hal-tinyusb.h"
#include "tusb.h"
#include "class/midi/midi_device.h"

namespace MIDIController {

static bool initialized = false;
constexpr uint8_t MAX_NOTE_OFFS = 16;
static NoteOffEvent noteOffQueue[MAX_NOTE_OFFS];
static uint8_t noteOffCount = 0;

// Descriptor callback para la interfaz MIDI (TinyUSB)
extern "C" uint16_t tud_midi_desc_cb(uint8_t* dst, uint8_t* itf) {
    uint8_t str_index = tinyusb_add_string_descriptor("GrooveDrum MIDI");
    uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
    if (ep_num == 0) return 0;

    uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
        TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_num, static_cast<uint8_t>(0x80 | ep_num), 64)
    };
    *itf += 1;
    memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
    return TUD_MIDI_DESC_LEN;
}

static uint8_t clampChannel(uint8_t ch) {
    return (ch < 1) ? 1 : ((ch > 16) ? 16 : ch);
}

void begin() {
    if (initialized) return;

    Serial.println("[MIDI] Initializing USB MIDI (USB native)...");

    USB.productName("GrooveDrum MIDI");
    USB.manufacturerName("GrooveDrum");
    USB.serialNumber("0001");

    tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tud_midi_desc_cb);
    USB.begin();

    uint32_t timeout = millis() + 5000;
    while (!tud_midi_mounted() && millis() < timeout) {
        delay(20);
    }

    initialized = true;
    noteOffCount = 0;

    Serial.println("[MIDI] USB MIDI initialized (USB native)");
    Serial.printf("[MIDI] Device mounted: %s\n", tud_midi_mounted() ? "Yes" : "No");
    Serial.printf("[MIDI] Default channel: %d\n", MIDI_CHANNEL);
}

void sendNoteOn(uint8_t note, uint8_t velocity) {
    sendNoteOn(MIDI_CHANNEL, note, velocity);
}

void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!isConnected()) return;

    uint8_t ch = clampChannel(channel);
    uint8_t msg[3] = {
        static_cast<uint8_t>(0x90 | ((ch - 1) & 0x0F)),
        static_cast<uint8_t>(note & 0x7F),
        static_cast<uint8_t>(velocity & 0x7F)
    };
    tud_midi_stream_write(0, msg, sizeof(msg));

    if (noteOffCount < MAX_NOTE_OFFS) {
        noteOffQueue[noteOffCount++] = {
            static_cast<uint8_t>(note & 0x7F),
            ch,
            millis() + NOTE_OFF_DURATION
        };
    }
}

void sendNoteOff(uint8_t note) {
    sendNoteOff(MIDI_CHANNEL, note);
}

void sendNoteOff(uint8_t channel, uint8_t note) {
    if (!isConnected()) return;

    uint8_t ch = clampChannel(channel);
    uint8_t msg[3] = {
        static_cast<uint8_t>(0x80 | ((ch - 1) & 0x0F)),
        static_cast<uint8_t>(note & 0x7F),
        0
    };
    tud_midi_stream_write(0, msg, sizeof(msg));
}

void sendControlChange(uint8_t control, uint8_t value) {
    sendControlChange(MIDI_CHANNEL, control, value);
}

void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    if (!isConnected()) return;

    uint8_t ch = clampChannel(channel);
    uint8_t msg[3] = {
        static_cast<uint8_t>(0xB0 | ((ch - 1) & 0x0F)),
        static_cast<uint8_t>(control & 0x7F),
        static_cast<uint8_t>(value & 0x7F)
    };
    tud_midi_stream_write(0, msg, sizeof(msg));
}

void update() {
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
                i++;
            }
        }
    }
}

bool isConnected() {
    return initialized && tud_midi_mounted();
}

} // namespace MIDIController
