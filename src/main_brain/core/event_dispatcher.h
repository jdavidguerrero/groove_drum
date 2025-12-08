#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "../input/hit_event.h"


// ============================================================================
// EVENT DISPATCHER - NON-BLOCKING ARCHITECTURE
// ============================================================================
// Routes hit events to multiple subsystems without blocking the scanner
// Uses separate FreeRTOS tasks for each heavy operation

// LED animation request
struct LEDRequest {
    uint8_t padId;
    uint32_t color;
    uint8_t brightness;
    uint16_t fadeDuration;
};

// Audio playback request
struct AudioRequest {
    char sampleName[32];
    uint8_t velocity;
    uint8_t volume;
    int8_t pitch;
};

// MIDI output request
struct MIDIRequest {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    bool noteOn;  // true=noteOn, false=noteOff
};

// ============================================================================
// EVENT DISPATCHER CLASS
// ============================================================================

class EventDispatcher {
public:
    // Initialize all subsystem tasks
    static void begin();

    // Main event handler (call from loop)
    static void processEvents();

    // Manual dispatch (for testing)
    static void dispatchLED(const LEDRequest& request);
    static void dispatchAudio(const AudioRequest& request);
    static void dispatchMIDI(const MIDIRequest& request);

    // Statistics
    static uint32_t getProcessedCount() { return processedCount; }
    static uint32_t getDroppedCount() { return droppedCount; }

private:
    // FreeRTOS queues (non-blocking)
    static QueueHandle_t hitQueue;
    static QueueHandle_t ledQueue;
    static QueueHandle_t audioQueue;
    static QueueHandle_t midiQueue;

    // Task handles
    static TaskHandle_t ledTaskHandle;
    static TaskHandle_t audioTaskHandle;
    static TaskHandle_t midiTaskHandle;

    // Task functions (run on Core 1)
    static void ledTask(void* parameter);
    static void audioTask(void* parameter);
    static void midiTask(void* parameter);

    // Statistics
    static volatile uint32_t processedCount;
    static volatile uint32_t droppedCount;
};

// ============================================================================
// SUBSYSTEM MODULES (to be implemented)
// ============================================================================

// LED Controller
namespace LEDController {
    void begin();
    void setColor(uint8_t padId, uint32_t color, uint8_t brightness);
    void fade(uint8_t padId, uint16_t duration);
    void update();  // Call from loop for smooth fading
}

// Audio Player
namespace AudioPlayer {
    void begin();
    bool playSample(const char* filename, uint8_t velocity, uint8_t volume, int8_t pitch);
    void stop(uint8_t padId);
    bool isPlaying();
}

// MIDI Controller
namespace MIDIController {
    void begin();
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note);
    void sendCC(uint8_t channel, uint8_t cc, uint8_t value);
}

#endif // EVENT_DISPATCHER_H
