#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

#include <Arduino.h>
#include <esp_task_wdt.h>

// ============================================================================
// SYSTEM WATCHDOG - MONITOR CRITICAL TASKS
// ============================================================================
// Monitors scanner task execution time and triggers recovery if hung
// Also monitors system health (heap, temperature, etc.)

namespace SystemWatchdog {

// Watchdog configuration
struct WatchdogConfig {
    uint32_t scannerTimeoutUs;      // Max allowed scanner execution time
    uint32_t heapWarningBytes;       // Warn if free heap below this
    uint32_t psramWarningBytes;      // Warn if free PSRAM below this
    int16_t tempWarningCelsius;      // Warn if temperature above this
    int16_t tempCriticalCelsius;     // Critical temperature (reboot)
};

// System health status
struct SystemHealth {
    uint32_t freeHeap;
    uint32_t freePSRAM;
    int16_t temperatureCelsius;
    uint32_t scannerMaxTime;
    uint32_t scannerMissedDeadlines;
    uint32_t uptimeSeconds;
    bool isHealthy;
};

// Initialize watchdog with configuration
void begin(const WatchdogConfig& config);

// Update watchdog (call from main loop, ~1Hz)
void update();

// Report scanner execution time (called from scanner)
void reportScannerTime(uint32_t executionTimeUs);

// Get system health
SystemHealth getHealth();

// Print health report
void printHealth();

// Manual recovery trigger
void triggerRecovery(const char* reason);

}  // namespace SystemWatchdog

#endif  // SYSTEM_WATCHDOG_H
