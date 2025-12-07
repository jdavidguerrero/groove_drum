#include "event_dispatcher.h"
#include "pad_config.h"
#include "uart_protocol.h"
#include "neopixel_controller.h"
#include "sk9822_controller.h"
#include "midi_controller.h"

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
        strncpy(audioReq.sampleName, cfg.sampleName, 31);
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
        // Block until audio request available
        if (xQueueReceive(audioQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Play sample (this may take time for SD card read)
            bool success = AudioPlayer::playSample(
                request.sampleName,
                request.velocity,
                request.volume,
                request.pitch
            );

            if (!success) {
                #ifdef DEBUG_VERBOSE
                Serial.printf("[AUDIO] Failed to play: %s\n", request.sampleName);
                #endif
            }
        }

        vTaskDelay(1);
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

void EventDispatcher::dispatchHit(const HitEvent& event) {
    xQueueSend(hitQueue, &event, 0);
}

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
