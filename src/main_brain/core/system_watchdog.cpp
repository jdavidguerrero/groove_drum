#include "system_watchdog.h"
#include <esp_system.h>

namespace SystemWatchdog {

// State
static WatchdogConfig config;
static SystemHealth health;
static uint32_t lastUpdateTime = 0;
static uint32_t lastWarningTime = 0;
static const uint32_t WARNING_COOLDOWN_MS = 5000;  // Don't spam warnings

// Statistics
static uint32_t totalWarnings = 0;
static uint32_t totalRecoveries = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void begin(const WatchdogConfig& cfg) {
    config = cfg;

    // Initialize ESP32 task watchdog
    esp_task_wdt_init(30, true);  // 30s timeout, panic on timeout

    Serial.println("[WATCHDOG] System watchdog initialized");
    Serial.printf("  Scanner timeout:   %lu µs\n", config.scannerTimeoutUs);
    Serial.printf("  Heap warning:      %lu bytes\n", config.heapWarningBytes);
    Serial.printf("  PSRAM warning:     %lu bytes\n", config.psramWarningBytes);
    Serial.printf("  Temp warning:      %d °C\n", config.tempWarningCelsius);
    Serial.printf("  Temp critical:     %d °C\n", config.tempCriticalCelsius);
}

// ============================================================================
// UPDATE (call from main loop)
// ============================================================================

void update() {
    uint32_t now = millis();
    if (now - lastUpdateTime < 1000) return;  // Update every 1s
    lastUpdateTime = now;

    // Update health metrics
    health.freeHeap = ESP.getFreeHeap();
    health.freePSRAM = ESP.getFreePsram();
    health.temperatureCelsius = (int16_t)(temperatureRead() * 10) / 10;
    health.uptimeSeconds = now / 1000;
    health.isHealthy = true;

    // Check heap
    if (health.freeHeap < config.heapWarningBytes) {
        if (now - lastWarningTime > WARNING_COOLDOWN_MS) {
            Serial.printf("[WATCHDOG] ⚠️  LOW HEAP: %lu bytes free\n", health.freeHeap);
            lastWarningTime = now;
            totalWarnings++;
        }
        health.isHealthy = false;
    }

    // Check PSRAM
    if (health.freePSRAM < config.psramWarningBytes) {
        if (now - lastWarningTime > WARNING_COOLDOWN_MS) {
            Serial.printf("[WATCHDOG] ⚠️  LOW PSRAM: %lu bytes free\n", health.freePSRAM);
            lastWarningTime = now;
            totalWarnings++;
        }
        health.isHealthy = false;
    }

    // Check temperature
    if (health.temperatureCelsius > config.tempCriticalCelsius) {
        triggerRecovery("CRITICAL TEMPERATURE");
    } else if (health.temperatureCelsius > config.tempWarningCelsius) {
        if (now - lastWarningTime > WARNING_COOLDOWN_MS) {
            Serial.printf("[WATCHDOG] ⚠️  HIGH TEMPERATURE: %d °C\n", health.temperatureCelsius);
            lastWarningTime = now;
            totalWarnings++;
        }
        health.isHealthy = false;
    }

    // Check scanner performance
    if (health.scannerMaxTime > config.scannerTimeoutUs) {
        if (now - lastWarningTime > WARNING_COOLDOWN_MS) {
            Serial.printf("[WATCHDOG] ⚠️  SCANNER SLOW: %lu µs (target: %lu µs)\n",
                          health.scannerMaxTime, config.scannerTimeoutUs);
            lastWarningTime = now;
            totalWarnings++;
        }
        health.isHealthy = false;
    }

    // Reset task watchdog
    esp_task_wdt_reset();
}

// ============================================================================
// SCANNER MONITORING
// ============================================================================

void reportScannerTime(uint32_t executionTimeUs) {
    if (executionTimeUs > health.scannerMaxTime) {
        health.scannerMaxTime = executionTimeUs;
    }

    if (executionTimeUs > config.scannerTimeoutUs) {
        health.scannerMissedDeadlines++;
    }
}

// ============================================================================
// GETTERS
// ============================================================================

SystemHealth getHealth() {
    return health;
}

void printHealth() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║       SYSTEM HEALTH REPORT             ║");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.printf("║ Status:        %s                  ║\n",
                  health.isHealthy ? "✓ HEALTHY " : "⚠ WARNING ");
    Serial.println("╟────────────────────────────────────────╢");
    Serial.printf("║ Free Heap:     %6lu KB             ║\n", health.freeHeap / 1024);
    Serial.printf("║ Free PSRAM:    %6lu KB             ║\n", health.freePSRAM / 1024);
    Serial.printf("║ Temperature:   %4d °C              ║\n", health.temperatureCelsius);
    Serial.printf("║ Uptime:        %6lu s              ║\n", health.uptimeSeconds);
    Serial.println("╟────────────────────────────────────────╢");
    Serial.printf("║ Scanner max:   %6lu µs             ║\n", health.scannerMaxTime);
    Serial.printf("║ Missed deadlines: %6lu            ║\n", health.scannerMissedDeadlines);
    Serial.println("╟────────────────────────────────────────╢");
    Serial.printf("║ Total warnings:   %6lu            ║\n", totalWarnings);
    Serial.printf("║ Total recoveries: %6lu            ║\n", totalRecoveries);
    Serial.println("╚════════════════════════════════════════╝\n");
}

// ============================================================================
// RECOVERY
// ============================================================================

void triggerRecovery(const char* reason) {
    totalRecoveries++;

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║     SYSTEM RECOVERY TRIGGERED          ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("Reason: %s\n", reason);
    Serial.printf("Uptime: %lu seconds\n", health.uptimeSeconds);
    Serial.println("\nSaving config to NVS...");

    // Save current configuration before reboot
    #include "pad_config.h"
    PadConfigManager::saveToNVS();

    Serial.println("Rebooting in 3 seconds...");
    delay(3000);

    ESP.restart();
}

}  // namespace SystemWatchdog
