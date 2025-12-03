/**
 * @file main.cpp
 * @brief Main entry point for E-Drum Controller - MCU #1 (Main Brain)
 * @version 1.0
 * @date 2025-12-02
 *
 * This firmware implements the data acquisition and processing for an
 * electronic drum controller with the following features:
 *
 * STAGE 1 (CURRENT):
 * - 4× Piezo trigger detection with velocity sensitivity
 * - Real-time peak detection algorithm (<2ms latency)
 * - Crosstalk rejection
 * - Serial console output for debugging
 *
 * FUTURE STAGES:
 * - STAGE 2: LED animations (WS2812B + SK9822)
 * - STAGE 3: MIDI output
 * - STAGE 4: GUI on MCU#2
 * - STAGE 5: I2S audio generation
 *
 * FreeRTOS Architecture:
 * - Core 0: Real-time trigger scanning (highest priority)
 * - Core 1: Application tasks (MIDI, LEDs, communication)
 */

#include <Arduino.h>
#include "../shared/config/edrum_config.h"
#include "core/system_config.h"
#include "input/trigger_scanner.h"
#include "input/trigger_detector.h"

// ============================================================
// FREERTOS HANDLES
// ============================================================

// Task handles
TaskHandle_t taskHandle_TriggerScan = NULL;
TaskHandle_t taskHandle_EventProcessor = NULL;

// Queue handles
QueueHandle_t queue_HitEvents = NULL;

// ============================================================
// TASK: EVENT PROCESSOR (Core 1)
// ============================================================

/**
 * @brief Process hit events from trigger detector
 * This task runs on Core 1 and handles hit events from the queue.
 * Currently prints to serial; will send MIDI in Stage 3.
 */
void eventProcessorTask(void* parameter) {
    HitEvent event;

    Serial.println("[eventProcessorTask] Started on Core 1");
    Serial.printf("  Priority: %d\n", uxTaskPriorityGet(NULL));

    while (true) {
        // Wait for hit events from queue
        if (xQueueReceive(queue_HitEvents, &event, portMAX_DELAY)) {
            // Print hit event to serial
            Serial.printf(">> HIT: Pad=%d (%s), Velocity=%d, Time=%lu µs\n",
                          event.padId,
                          PAD_NAMES[event.padId],
                          event.velocity,
                          event.timestamp);

            // STAGE 3 TODO: Send MIDI Note On
            // STAGE 2 TODO: Trigger LED flash animation
            // STAGE 4 TODO: Send event to MCU#2 via UART
        }
    }
}

// ============================================================
// SERIAL COMMAND HANDLER
// ============================================================

/**
 * @brief Handle serial commands for debugging and configuration
 */
void handleSerialCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case 'h':
            case 'H':
                // Print help
                Serial.println("\n=== E-Drum Controller - Serial Commands ===");
                Serial.println("h - Show this help");
                Serial.println("s - Show system info");
                Serial.println("t - Show trigger detector state");
                Serial.println("a - Show ADC test");
                Serial.println("r - Reset all triggers");
                Serial.println("m - Show scan timing stats");
                Serial.println("c - Clear scan stats");
                Serial.println("===========================================\n");
                break;

            case 's':
            case 'S':
                // Show system info
                printSystemInfo();
                break;

            case 't':
            case 'T':
                // Show trigger state
                triggerDetector.printState();
                break;

            case 'a':
            case 'A':
                // Test ADC
                testADC();
                break;

            case 'r':
            case 'R':
                // Reset triggers
                triggerDetector.resetAll();
                break;

            case 'm':
            case 'M':
                // Show scan stats
                triggerScanner.printStats();
                break;

            case 'c':
            case 'C':
                // Clear scan stats
                triggerScanner.resetStats();
                break;

            default:
                Serial.printf("Unknown command: '%c'. Press 'h' for help.\n", cmd);
                break;
        }
    }
}

// ============================================================
// ARDUINO SETUP
// ============================================================

void setup() {
    // Initialize serial for debug output
    Serial.begin(DEBUG_BAUD_RATE);
    delay(500);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║   E-DRUM CONTROLLER - MAIN BRAIN      ║");
    Serial.println("║   Firmware v" FIRMWARE_VERSION "                   ║");
    Serial.println("║   Build: " __DATE__ " " __TIME__ "  ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // ⚠️ CRITICAL HARDWARE WARNING
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║          ⚠️  HARDWARE WARNING ⚠️         ║");
    Serial.println("║                                        ║");
    Serial.println("║  ENSURE PIEZO PROTECTION CIRCUITS ARE  ║");
    Serial.println("║  INSTALLED BEFORE CONNECTING PIEZOS!   ║");
    Serial.println("║                                        ║");
    Serial.println("║  Direct piezo connection WILL DESTROY  ║");
    Serial.println("║  the ESP32-S3 ADC!                     ║");
    Serial.println("║                                        ║");
    Serial.println("║  See docs/hardware_assembly.md         ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    delay(1000);

    // Initialize system hardware
    if (!systemInit()) {
        Serial.println("\n[FATAL ERROR] System initialization failed!");
        Serial.println("System halted. Check hardware connections.");
        while (true) {
            delay(1000);
        }
    }

    // Create FreeRTOS queues
    Serial.println("\n--- Creating FreeRTOS Queues ---");
    queue_HitEvents = xQueueCreate(QUEUE_SIZE_HIT_EVENTS, sizeof(HitEvent));
    if (queue_HitEvents == NULL) {
        Serial.println("[FATAL ERROR] Failed to create hit events queue!");
        while (true) delay(1000);
    }
    Serial.printf("[OK] Hit events queue created (size: %d)\n", QUEUE_SIZE_HIT_EVENTS);

    // Initialize trigger scanner
    if (!triggerScanner.begin(queue_HitEvents)) {
        Serial.println("[FATAL ERROR] Trigger scanner initialization failed!");
        while (true) delay(1000);
    }

    // Create FreeRTOS tasks
    Serial.println("\n--- Creating FreeRTOS Tasks ---");

    // Core 0: Trigger scanning (highest priority)
    BaseType_t result = xTaskCreatePinnedToCore(
        triggerScanTask,              // Task function
        "TriggerScan",                // Name
        TASK_STACK_TRIGGER_SCAN,      // Stack size
        NULL,                         // Parameters
        TASK_PRIORITY_TRIGGER_SCAN,   // Priority (24 - highest)
        &taskHandle_TriggerScan,      // Handle
        TASK_CORE_TRIGGER_SCAN        // Core 0
    );

    if (result != pdPASS) {
        Serial.println("[FATAL ERROR] Failed to create trigger scan task!");
        while (true) delay(1000);
    }
    Serial.println("[OK] Trigger scan task created on Core 0");

    // Core 1: Event processor
    result = xTaskCreatePinnedToCore(
        eventProcessorTask,           // Task function
        "EventProcessor",             // Name
        4096,                         // Stack size
        NULL,                         // Parameters
        10,                           // Priority
        &taskHandle_EventProcessor,   // Handle
        1                             // Core 1
    );

    if (result != pdPASS) {
        Serial.println("[FATAL ERROR] Failed to create event processor task!");
        while (true) delay(1000);
    }
    Serial.println("[OK] Event processor task created on Core 1");

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  SYSTEM READY - STAGE 1: PAD READING   ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Press 'h' for serial commands help");
    Serial.println();

    // Print current task info
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Tasks running: %d\n\n", uxTaskGetNumberOfTasks());
}

// ============================================================
// ARDUINO LOOP
// ============================================================

/**
 * @brief Main loop (runs on Core 1)
 * Since we're using FreeRTOS, this loop is minimal.
 * Most work is done in tasks.
 */
void loop() {
    // Handle serial commands for debugging
    handleSerialCommands();

    // Small delay to prevent watchdog issues
    delay(10);

    // STAGE 2 TODO: Update LED animations here
    // STAGE 3 TODO: Handle MIDI note-off timing
    // STAGE 4 TODO: Handle UART communication with MCU#2
}
