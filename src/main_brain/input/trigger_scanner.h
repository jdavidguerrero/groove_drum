/**
 * @file trigger_scanner.h
 * @brief High-priority ADC scanning task for trigger detection
 * @version 1.0
 * @date 2025-12-02
 *
 * This module implements a real-time ADC scanning loop that runs on Core 0
 * with highest priority. It reads all 4 piezo sensors at 2kHz (500µs period)
 * and passes samples to the trigger detector.
 *
 * Task Configuration:
 * - Core: 0 (dedicated real-time core)
 * - Priority: 24 (highest)
 * - Stack: 4KB
 * - Period: 500µs (2kHz scan rate)
 */

#pragma once

#include <Arduino.h>
#include <edrum_config.h>
#include "trigger_detector.h"

// ============================================================
// TRIGGER SCANNER CLASS
// ============================================================

class TriggerScanner {
public:
    /**
     * @brief Constructor
     */
    TriggerScanner();

    /**
     * @brief Initialize the trigger scanner
     * @param hitQueue FreeRTOS queue for hit events
     * @return true if initialization successful
     */
    bool begin(QueueHandle_t hitQueue);

    /**
     * @brief Get statistics about scan timing
     * @param avgUs Average scan time in microseconds
     * @param maxUs Maximum scan time in microseconds
     * @param minUs Minimum scan time in microseconds
     */
    void getStats(uint32_t& avgUs, uint32_t& maxUs, uint32_t& minUs);

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Print current statistics
     */
    void printStats();

    /**
     * @brief Main scanning loop (called by FreeRTOS task)
     */
    void scanLoop();

private:
    bool initialized;

    // Timing statistics
    uint32_t scanCount;
    uint32_t totalScanTimeUs;
    uint32_t maxScanTimeUs;
    uint32_t minScanTimeUs;
    uint32_t lastStatsTime;

    /**
     * @brief Read all pads and process samples
     */
    void readAllPads();

    /**
     * @brief Update timing statistics
     * @param scanTimeUs Time taken for this scan
     */
    void updateStats(uint32_t scanTimeUs);
};

// ============================================================
// GLOBAL INSTANCE
// ============================================================

extern TriggerScanner triggerScanner;

// ============================================================
// SCANNER CONTROL FUNCTIONS
// ============================================================

/**
 * @brief Start high-precision scanner using esp_timer (2kHz exact)
 * Replaces FreeRTOS task for better timing precision
 */
void startTriggerScanner();

/**
 * @brief Stop the trigger scanner
 */
void stopTriggerScanner();

/**
 * @brief Get number of missed deadlines (execution > 500µs)
 * @return Count of times scanner exceeded target period
 */
uint32_t getMissedDeadlines();
