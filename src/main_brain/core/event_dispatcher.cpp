#include "event_dispatcher.h"
#include "pad_config.h"
#include <edrum_config.h>
#include "../output/audio_engine.h"
#include "../communication/uart_protocol.h"
#include "../ui/neopixel_controller.h"
#include "../ui/sk9822_controller.h"
#include "../output/midi_controller.h"
#include <algorithm>
#include <cstring>

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
    audioQueue = xQueueCreate(16, sizeof(AudioRequest)); // Increased audio buffer
    midiQueue = xQueueCreate(32, sizeof(MIDIRequest));  // 32 MIDI buffer

    // Initialize subsystems (if not already init in main)
    // Note: AudioEngine and MIDIController should be init in main.cpp for explicit ordering
    
    // Create worker tasks on Core 1 (application core)
    xTaskCreatePinnedToCore(
        ledTask,
        "LED_Task",
        2048,           // Stack size
        nullptr,
        5,              // Priority (Low)
        &ledTaskHandle,
        1               // Core 1
    );

    xTaskCreatePinnedToCore(
        audioTask,
        "Audio_Task",
        2048,           // Reduced stack as heavy lifting is in AudioEngine
        nullptr,
        15,             // Priority (High - audio dispatch must be fast)
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
}

// ============================================================================
// MAIN EVENT PROCESSOR (Called from main loop)
// ============================================================================

void EventDispatcher::processEvents() {
    // Legacy function - kept for compatibility but main logic moved to tasks
    // Or used if you want to poll from loop() instead of tasks
}

// ============================================================================
// MANUAL DISPATCH
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
// PROCESS AUDIO IN MAIN LOOP
// ============================================================================

// Helper to drain the queue in the main loop if tasks are not used for this
void EventDispatcher::processAudio() {
    AudioRequest req;
    while (xQueueReceive(audioQueue, &req, 0) == pdTRUE) {
        
        // Determinar Choke Group
        // TODO: Mover esta lógica a PadConfigManager
        uint8_t chokeGroup = 0;
        
        // HiHat logic (simple hardcoded logic for now)
        // If "closed hihat" or "pedal", choke group 1
        if (strstr(req.sampleName, "hihat") != nullptr) {
            chokeGroup = 1;
        }

        AudioEngine::play(req.sampleName, req.velocity, req.volume, chokeGroup);
    }
}

// ============================================================================
// WORKER TASKS
// ============================================================================

void EventDispatcher::ledTask(void* parameter) {
    LEDRequest request;
    while (true) {
        if (xQueueReceive(ledQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            NeoPixelController::flashPad(request.padId, request.color, request.brightness, request.fadeDuration);
        }
        vTaskDelay(1);
    }
}

void EventDispatcher::audioTask(void* parameter) {
    // Not strictly needed if processAudio() is called in loop, 
    // but useful if we want to decouple completely.
    // For now, let's just yield as we are using processAudio() in loop() for lower latency jitter.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void EventDispatcher::midiTask(void* parameter) {
    MIDIRequest request;
    while (true) {
        if (xQueueReceive(midiQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (request.noteOn) {
                MIDIController::sendNoteOn(request.note, request.velocity);
            } else {
                MIDIController::sendNoteOff(request.note);
            }
        }
        vTaskDelay(1);
    }
}