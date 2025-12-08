#include "event_dispatcher.h"
#include "pad_config.h"
#include <edrum_config.h>
#include "audio_mixer.h"
#include "uart_protocol.h"
#include "neopixel_controller.h"
#include "sk9822_controller.h"
#include "midi_controller.h"
#include "audio_samples.h"
#include <driver/i2s.h>
#include <algorithm>

// Static members
QueueHandle_t EventDispatcher::hitQueue = nullptr;
QueueHandle_t EventDispatcher::ledQueue = nullptr;
QueueHandle_t EventDispatcher::audioQueue = nullptr;
QueueHandle_t EventDispatcher::midiQueue = nullptr;

TaskHandle_t EventDispatcher::ledTaskHandle = nullptr;
TaskHandle_t EventDispatcher::audioTaskHandle = nullptr;
TaskHandle_t EventDispatcher::midiTaskHandle = nullptr;

volatile uint32_t EventDispatcher::processedCount = 0;
volatile uint32_t EventDispatcher::droppedCount = 0;

namespace {

constexpr size_t AUDIO_CHUNK_FRAMES = 128;
constexpr size_t AUDIO_MAX_VOICES = 4;
const char* DEFAULT_PAD_SAMPLE_PATHS[NUM_PADS] = {
    SAMPLE_PATH_KICK,
    SAMPLE_PATH_SNARE,
    SAMPLE_PATH_HIHAT,
    SAMPLE_PATH_TOM
};

struct ActiveVoice {
    const Sample* sample = nullptr;
    uint32_t position = 0;
    float gain = 0.0f;
    uint8_t channels = 1;
    bool active = false;
};

float computeGain(uint8_t velocity, uint8_t volume) {
    float vel = std::max(1u, (unsigned)velocity) / 127.0f;
    float vol = std::max(1u, (unsigned)volume) / 127.0f;
    return std::max(0.01f, vel * vol);
}

void startVoice(const AudioRequest& request, ActiveVoice* voices) {
    const Sample* sample = SampleManager::getSample(request.sampleName);
    if (!sample || !sample->data || sample->frames == 0) {
        return;
    }

    ActiveVoice* slot = nullptr;
    for (size_t i = 0; i < AUDIO_MAX_VOICES; ++i) {
        if (!voices[i].active) {
            slot = &voices[i];
            break;
        }
    }
    if (!slot) {
        slot = &voices[0];  // simple voice steal
    }

    slot->sample = sample;
    slot->position = 0;
    slot->gain = computeGain(request.velocity, request.volume);
    slot->channels = sample->channels;
    slot->active = true;
}

bool hasActiveVoices(const ActiveVoice* voices) {
    for (size_t i = 0; i < AUDIO_MAX_VOICES; ++i) {
        if (voices[i].active) return true;
    }
    return false;
}

void mixVoices(ActiveVoice* voices, int16_t* outBuf, size_t frames) {
    for (size_t i = 0; i < frames; ++i) {
        int32_t mixL = 0;
        int32_t mixR = 0;

        for (size_t v = 0; v < AUDIO_MAX_VOICES; ++v) {
            ActiveVoice& voice = voices[v];
            if (!voice.active || !voice.sample) continue;

            if (voice.position >= voice.sample->frames) {
                voice.active = false;
                continue;
            }

            const int16_t* data = voice.sample->data;
            int16_t left = 0;
            int16_t right = 0;

            if (voice.channels == 1) {
                int16_t val = data[voice.position];
                left = val;
                right = val;
            } else {
                uint32_t idx = voice.position * 2;
                left = data[idx];
                right = data[idx + 1];
            }

            mixL += static_cast<int32_t>(left * voice.gain);
            mixR += static_cast<int32_t>(right * voice.gain);

            voice.position++;
        }

        mixL = std::max(-32768, std::min(32767, mixL));
        mixR = std::max(-32768, std::min(32767, mixR));

        outBuf[2 * i] = static_cast<int16_t>(mixL);
        outBuf[2 * i + 1] = static_cast<int16_t>(mixR);
    }
}

} // namespace

// ============================================================================
// INITIALIZATION
// ============================================================================

void EventDispatcher::begin() {
    // Create queues (non-blocking)
    hitQueue = xQueueCreate(32, sizeof(HitEvent));      // 32 hit buffer
    ledQueue = xQueueCreate(16, sizeof(LEDRequest));    // 16 LED commands
    audioQueue = xQueueCreate(8, sizeof(AudioRequest)); // 8 audio buffer
    midiQueue = xQueueCreate(32, sizeof(MIDIRequest));  // 32 MIDI buffer

    // Initialize subsystems
    LEDController::begin();
    AudioMixer_begin();
    AudioPlayer::begin();
    MIDIController::begin();

    // Create worker tasks on Core 1 (application core)
    xTaskCreatePinnedToCore(
        ledTask,
        "LED_Task",
        2048,           // Stack size
        nullptr,
        5,              // Priority (lower than scanner)
        &ledTaskHandle,
        1               // Core 1
    );

    xTaskCreatePinnedToCore(
        audioTask,
        "Audio_Task",
        4096,           // Larger stack for file I/O
        nullptr,
        6,              // Higher priority (audio is time-sensitive)
        &audioTaskHandle,
        1
    );

    xTaskCreatePinnedToCore(
        midiTask,
        "MIDI_Task",
        2048,
        nullptr,
        5,
        &midiTaskHandle,
        1
    );

    Serial.println("[DISPATCHER] Event dispatcher initialized");
    Serial.println("[DISPATCHER] LED, Audio, MIDI tasks running on Core 1");
}

// ============================================================================
// MAIN EVENT PROCESSOR (call from loop)
// ============================================================================

void EventDispatcher::processEvents() {
    HitEvent event;

    // Non-blocking receive from scanner
    while (xQueueReceive(hitQueue, &event, 0) == pdTRUE) {
        processedCount++;

        // Get pad configuration
        PadConfig& cfg = PadConfigManager::getConfig(event.padId);

        // Skip if pad disabled
        if (!cfg.enabled) continue;

        // 1. Dispatch to LED (non-blocking)
        LEDRequest ledReq = {
            .padId = event.padId,
            .color = cfg.ledColorHit,
            .brightness = cfg.ledBrightness,
            .fadeDuration = cfg.ledFadeDuration
        };
        if (xQueueSend(ledQueue, &ledReq, 0) != pdTRUE) {
            droppedCount++;
        }

        // 2. Dispatch to Audio (non-blocking)
        AudioRequest audioReq;
        const char* path = DEFAULT_PAD_SAMPLE_PATHS[event.padId % NUM_PADS];
        strncpy(audioReq.sampleName, path, 31);
        audioReq.sampleName[31] = '\0';
        audioReq.velocity = event.velocity;
        audioReq.volume = cfg.sampleVolume;
        audioReq.pitch = cfg.samplePitch;

        if (xQueueSend(audioQueue, &audioReq, 0) != pdTRUE) {
            droppedCount++;
        }

        // 3. Dispatch to MIDI (non-blocking)
        MIDIRequest midiReq = {
            .channel = cfg.midiChannel,
            .note = cfg.midiNote,
            .velocity = event.velocity,
            .noteOn = true
        };
        if (xQueueSend(midiQueue, &midiReq, 0) != pdTRUE) {
            droppedCount++;
        }

        // Schedule MIDI note off after 50ms (typical drum note duration)
        // This could be improved with a timer-based approach
        MIDIRequest midiOffReq = midiReq;
        midiOffReq.noteOn = false;
        // TODO: Queue this with delay, or use separate timer

        // 4. Send telemetry to GUI (non-blocking)
        UARTProtocol::sendHitEvent(event.padId, event.velocity,
                                    event.timestamp, event.peakValue);

        // 5. Serial debug (keep minimal to avoid blocking)
        #ifdef DEBUG_VERBOSE
        Serial.printf("HIT: %s | VEL: %3d | PEAK: %4d | TIME: %lu\n",
                      cfg.name, event.velocity, event.peakValue, event.timestamp);
        #endif
    }

    // Update LED animations (smooth fading in main loop)
    LEDController::update();
}

// ============================================================================
// WORKER TASKS
// ============================================================================

void EventDispatcher::ledTask(void* parameter) {
    LEDRequest request;

    while (true) {
        // Block until LED request available (timeout 100ms)
        if (xQueueReceive(ledQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Set LED color immediately
            LEDController::setColor(request.padId, request.color, request.brightness);

            // Start fade (handled by LEDController::update() in main loop)
            LEDController::fade(request.padId, request.fadeDuration);
        }

        // Small yield to prevent watchdog
        vTaskDelay(1);
    }
}

void EventDispatcher::audioTask(void* parameter) {
    AudioRequest request;

    while (true) {
        while (xQueueReceive(audioQueue, &request, 0) == pdTRUE) {
            AudioMixer_startVoice(request);
        }

        AudioMixer_update();
    }
}

void EventDispatcher::midiTask(void* parameter) {
    MIDIRequest request;

    while (true) {
        // Block until MIDI request available
        if (xQueueReceive(midiQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (request.noteOn) {
                MIDIController::sendNoteOn(request.channel, request.note, request.velocity);
            } else {
                MIDIController::sendNoteOff(request.channel, request.note);
            }
        }

        vTaskDelay(1);
    }
}

// ============================================================================
// MANUAL DISPATCH (for testing)
// ============================================================================

void EventDispatcher::dispatchLED(const LEDRequest& request) {
    xQueueSend(ledQueue, &request, 0);
}

void EventDispatcher::dispatchAudio(const AudioRequest& request) {
    xQueueSend(audioQueue, &request, 0);
}

void EventDispatcher::dispatchMIDI(const MIDIRequest& request) {
    xQueueSend(midiQueue, &request, 0);
}

// ============================================================================
// STUB IMPLEMENTATIONS (to be completed)
// ============================================================================

namespace LEDController {
    void begin() {
        NeoPixelController::begin();
        EncoderLEDController::begin();
        Serial.println("[LED] Controller initialized");
    }

    void setColor(uint8_t padId, uint32_t color, uint8_t brightness) {
        // Flash pad LED with hit color
        NeoPixelController::flashPad(padId, color, brightness, 200);
    }

    void fade(uint8_t padId, uint16_t duration) {
        // Fade is handled automatically in NeoPixelController::update()
        (void)padId;
        (void)duration;
    }

    void update() {
        // Update both NeoPixels and SK9822
        NeoPixelController::update();
        EncoderLEDController::update();
    }
}

namespace AudioPlayer {
    void begin() {
        // TODO: Initialize I2S, SD card
        Serial.println("[AUDIO] Player initialized (stub)");
    }

    bool playSample(const char* filename, uint8_t velocity, uint8_t volume, int8_t pitch) {
        // TODO: Load WAV from SD, play via I2S with DMA
        // Apply velocity scaling, volume, pitch shift
        return false;  // Stub
    }

    void stop(uint8_t padId) {
        // TODO: Stop playing sample
    }

    bool isPlaying() {
        return false;  // Stub
    }
}

// MIDIController implementation lives in src/main_brain/output/midi_controller.*
