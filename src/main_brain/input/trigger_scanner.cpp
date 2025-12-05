/**
 * @file trigger_scanner.cpp
 * @brief Implementation of ADC trigger scanner with esp_timer (2kHz precision)
 * @version 2.0
 * @date 2025-12-04
 */

#include "trigger_scanner.h"
#include <esp_timer.h>

// Forward declaration of safety check function (defined in main app)
void checkADCSafety(uint16_t value, uint8_t padId);

// Global instance
TriggerScanner triggerScanner;

// esp_timer handle for high-precision scanning
static esp_timer_handle_t scanTimer = nullptr;
static volatile uint32_t missedDeadlines = 0;

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
    Serial.printf("Total Scans: %u\n", scanCount);
    Serial.printf("Avg Scan Time: %u µs\n", avgUs);
    Serial.printf("Max Scan Time: %u µs\n", maxUs);
    Serial.printf("Min Scan Time: %u µs\n", minUs);
    Serial.printf("Target Period: %d µs\n", SCAN_PERIOD_US);

    if (maxUs > SCAN_PERIOD_US) {
        Serial.printf("[WARNING] Max scan time exceeds target period by %u µs!\n",
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
// ESP_TIMER CALLBACK (High-Precision 2kHz)
// ============================================================

static void IRAM_ATTR scanTimerCallback(void* arg) {
    uint64_t startTime = esp_timer_get_time();

    // Execute scan loop
    triggerScanner.scanLoop();

    // Monitor execution time
    uint64_t executionTime = esp_timer_get_time() - startTime;

    // Check if we exceeded the target period (500µs)
    if (executionTime > SCAN_PERIOD_US) {
        missedDeadlines++;
    }
}

// ============================================================
// START FUNCTION (replaces FreeRTOS task)
// ============================================================

void startTriggerScanner() {
    // Create high-resolution timer
    esp_timer_create_args_t timerConfig = {
        .callback = &scanTimerCallback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "piezo_scan",
        .skip_unhandled_events = false
    };

    esp_err_t err = esp_timer_create(&timerConfig, &scanTimer);
    if (err != ESP_OK) {
        Serial.printf("[SCANNER] ERROR: Failed to create timer: %d\n", err);
        return;
    }

    // Start periodic timer at 2kHz (500µs)
    err = esp_timer_start_periodic(scanTimer, SCAN_PERIOD_US);
    if (err != ESP_OK) {
        Serial.printf("[SCANNER] ERROR: Failed to start timer: %d\n", err);
        return;
    }

    Serial.println("[SCANNER] High-precision scanner started");
    Serial.printf("  Target frequency: %d Hz\n", SCAN_RATE_HZ);
    Serial.printf("  Target period: %d µs\n", SCAN_PERIOD_US);
}

void stopTriggerScanner() {
    if (scanTimer) {
        esp_timer_stop(scanTimer);
        esp_timer_delete(scanTimer);
        scanTimer = nullptr;
        Serial.println("[SCANNER] Stopped");
    }
}

uint32_t getMissedDeadlines() {
    return missedDeadlines;
}
