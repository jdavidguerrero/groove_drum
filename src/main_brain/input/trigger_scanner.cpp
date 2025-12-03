/**
 * @file trigger_scanner.cpp
 * @brief Implementation of ADC trigger scanner
 * @version 1.0
 * @date 2025-12-02
 */

#include "trigger_scanner.h"
#include "../core/system_config.h"

// Global instance
TriggerScanner triggerScanner;

// ============================================================
// CONSTRUCTOR
// ============================================================

TriggerScanner::TriggerScanner() :
    initialized(false),
    scanCount(0),
    totalScanTimeUs(0),
    maxScanTimeUs(0),
    minScanTimeUs(0xFFFFFFFF),
    lastStatsTime(0) {
}

// ============================================================
// INITIALIZATION
// ============================================================

bool TriggerScanner::begin(QueueHandle_t hitQueue) {
    // Initialize trigger detector
    triggerDetector.begin(hitQueue);

    initialized = true;
    lastStatsTime = millis();

    Serial.println("[TriggerScanner] Initialized");
    Serial.printf("  Scan Rate: %d Hz\n", SCAN_RATE_HZ);
    Serial.printf("  Scan Period: %d µs\n", SCAN_PERIOD_US);

    return true;
}

// ============================================================
// MAIN SCANNING LOOP
// ============================================================

void TriggerScanner::scanLoop() {
    if (!initialized) {
        Serial.println("[ERROR] TriggerScanner not initialized!");
        return;
    }

    uint32_t scanStartUs = micros();

    // Read all pads
    readAllPads();

    // Update timing statistics
    uint32_t scanTimeUs = micros() - scanStartUs;
    updateStats(scanTimeUs);

    // Print stats every 10 seconds
    if (millis() - lastStatsTime > 10000) {
        #ifdef DEBUG_TRIGGER_TIMING
        printStats();
        #endif
        lastStatsTime = millis();
    }
}

// ============================================================
// PAD READING
// ============================================================

void TriggerScanner::readAllPads() {
    uint32_t timestamp = micros();

    // Read all 4 pads sequentially
    for (int pad = 0; pad < NUM_PADS; pad++) {
        uint16_t rawValue = analogRead(PAD_ADC_PINS[pad]);

        #ifdef DEBUG_TRIGGER_RAW
        if (scanCount % 1000 == 0) {  // Print every 1000 scans (~0.5 sec)
            Serial.printf("Pad %d: %d\n", pad, rawValue);
        }
        #endif

        // Safety check (protection circuit validation)
        checkADCSafety(rawValue, pad);

        // Process sample through trigger detector
        triggerDetector.processSample(pad, rawValue, timestamp);
    }
}

// ============================================================
// STATISTICS
// ============================================================

void TriggerScanner::updateStats(uint32_t scanTimeUs) {
    scanCount++;
    totalScanTimeUs += scanTimeUs;

    if (scanTimeUs > maxScanTimeUs) {
        maxScanTimeUs = scanTimeUs;
    }

    if (scanTimeUs < minScanTimeUs) {
        minScanTimeUs = scanTimeUs;
    }
}

void TriggerScanner::getStats(uint32_t& avgUs, uint32_t& maxUs, uint32_t& minUs) {
    if (scanCount > 0) {
        avgUs = totalScanTimeUs / scanCount;
    } else {
        avgUs = 0;
    }

    maxUs = maxScanTimeUs;
    minUs = (minScanTimeUs == 0xFFFFFFFF) ? 0 : minScanTimeUs;
}

void TriggerScanner::resetStats() {
    scanCount = 0;
    totalScanTimeUs = 0;
    maxScanTimeUs = 0;
    minScanTimeUs = 0xFFFFFFFF;

    Serial.println("[TriggerScanner] Statistics reset");
}

void TriggerScanner::printStats() {
    uint32_t avgUs, maxUs, minUs;
    getStats(avgUs, maxUs, minUs);

    Serial.println("--- Trigger Scanner Stats ---");
    Serial.printf("Total Scans: %lu\n", scanCount);
    Serial.printf("Avg Scan Time: %lu µs\n", avgUs);
    Serial.printf("Max Scan Time: %lu µs\n", maxUs);
    Serial.printf("Min Scan Time: %lu µs\n", minUs);
    Serial.printf("Target Period: %d µs\n", SCAN_PERIOD_US);

    if (maxUs > SCAN_PERIOD_US) {
        Serial.printf("[WARNING] Max scan time exceeds target period by %lu µs!\n",
                      maxUs - SCAN_PERIOD_US);
    }

    // Calculate actual scan rate
    if (avgUs > 0) {
        float actualRate = 1000000.0f / avgUs;
        Serial.printf("Actual Scan Rate: %.1f Hz\n", actualRate);
    }

    Serial.println("----------------------------");
}

// ============================================================
// FREERTOS TASK FUNCTION
// ============================================================

void triggerScanTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t scanPeriodTicks = pdUS_TO_TICKS(SCAN_PERIOD_US);

    Serial.println("[triggerScanTask] Started on Core 0");
    Serial.printf("  Priority: %d\n", uxTaskPriorityGet(NULL));
    Serial.printf("  Stack: %d bytes\n", TASK_STACK_TRIGGER_SCAN);

    while (true) {
        // Execute scan loop
        triggerScanner.scanLoop();

        // Wait until next scan period (maintains precise 2kHz rate)
        vTaskDelayUntil(&lastWakeTime, scanPeriodTicks);
    }
}
