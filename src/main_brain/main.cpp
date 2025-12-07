/**
 * @file main.cpp
 * @brief E-Drum Controller - Sistema Profesional de Trigger Detection
 * @version 2.0
 * @date 2025-12-04
 *
 * Sistema completo con:
 * - Peak detection con state machine
 * - Baseline tracking automático
 * - Threshold individual por pad
 * - Crosstalk rejection
 * - Velocity curve natural
 * - FreeRTOS real-time scanning (2kHz, Core 0)
 *
 * Comandos por Serial:
 *   's' - Mostrar estadísticas de scanner
 *   'd' - Mostrar estado del detector
 *   'c' - Calibrar thresholds (modo interactivo 30s)
 *   'r' - Reset sistema completo
 *   'h' - Ayuda
 */

#include <Arduino.h>
#include <edrum_config.h>
#include "input/trigger_scanner.h"
#include "input/trigger_detector.h"
#include "ui/neopixel_controller.h"
#include "output/midi_controller.h"

// ============================================================
// CONFIG ARRAY DEFINITIONS (from edrum_config.h)
// ============================================================

const int PAD_ADC_PINS[4] = {PAD0_ADC_PIN, PAD1_ADC_PIN, PAD2_ADC_PIN, PAD3_ADC_PIN};
const char* PAD_NAMES[4] = {"Kick", "Snare", "HiHat", "Tom"};

// Threshold por pad - Ajustar según ruido observado de cada piezo
const uint16_t TRIGGER_THRESHOLD_PER_PAD[4] = {
    250,  // Pad 0 (Kick)    - Ajustar si hay falsos positivos/negativos
    250,  // Pad 1 (Snare)   - Ajustar si hay falsos positivos/negativos
    200,  // Pad 2 (HiHat)   - Más sensible, threshold menor
    250   // Pad 3 (Tom)     - Ajustar si hay falsos positivos/negativos
};

// Rangos de velocity - Calibrar con el comando 'c'
const uint16_t VELOCITY_MIN_PEAK[4] = {
    100,  // Pad 0 (Kick)    - Golpe más suave que quieres detectar
    100,  // Pad 1 (Snare)
    80,   // Pad 2 (HiHat)   - Típicamente más sensible
    100   // Pad 3 (Tom)
};

const uint16_t VELOCITY_MAX_PEAK[4] = {
    3500,  // Pad 0 (Kick)    - Golpe más fuerte esperado
    3500,  // Pad 1 (Snare)
    3000,  // Pad 2 (HiHat)
    3500   // Pad 3 (Tom)
};

const uint8_t PAD_MIDI_NOTES[4] = {
    36,  // Pad 0: Kick (C1 / Bass Drum 1)
    38,  // Pad 1: Snare (D1 / Acoustic Snare)
    42,  // Pad 2: HiHat (F#1 / Closed Hi-Hat)
    48   // Pad 3: Tom (C2 / Hi-Mid Tom)
};

// Color idle universal (blanco azulado para todos los pads)
const CRGB PAD_LED_IDLE_COLOR = CRGB(10, 10, 10);  // Blanco con tinte azul suave

// Colores al golpear (asignables por usuario - por defecto: cian, rosa, amarillo, verde)
const CRGB PAD_LED_HIT_COLORS[4] = {
    CRGB(0, 255, 255),    // Pad 0: Kick - Cian
    CRGB(255, 50, 150),   // Pad 1: Snare - Rosa/Magenta
    CRGB(255, 255, 0),    // Pad 2: HiHat - Amarillo
    CRGB(0, 255, 100)     // Pad 3: Tom - Verde
};

// ============================================================
// GLOBAL VARIABLES
// ============================================================

// Queue for hit events (Trigger → MIDI/LED)
QueueHandle_t hitEventQueue;

// Calibration mode
bool calibrationMode = false;
uint32_t calibrationStartTime = 0;
uint16_t calibrationPeaks[4] = {0, 0, 0, 0};
uint16_t calibrationMins[4] = {4095, 4095, 4095, 4095};

// Statistics
uint32_t totalHitsDetected = 0;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void setupHardware();
void processHitEvents();
void handleSerialCommands();
void printHelp();
void printStats();
void printDetectorState();
void startCalibration();
void processCalibration();
void checkADCSafety(uint16_t value, uint8_t padId);

// ============================================================
// SETUP
// ============================================================

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════════════╗");
    Serial.println("║      E-DRUM CONTROLLER - PROFESSIONAL v2.0    ║");
    Serial.println("║          Sistema de Trigger Avanzado          ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
    Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
    Serial.println();

    // Setup hardware
    setupHardware();

    // Create hit event queue
    hitEventQueue = xQueueCreate(QUEUE_SIZE_HIT_EVENTS, sizeof(HitEvent));
    if (hitEventQueue == nullptr) {
        Serial.println("[ERROR] Failed to create hit event queue!");
        while (1) delay(1000);
    }
    Serial.println("[Queue] Hit event queue created");

    // Initialize trigger system
    Serial.println("\n[System] Initializing trigger detection...");
    triggerScanner.begin(hitEventQueue);

    // Start high-precision scanner with esp_timer (replaces FreeRTOS task)
    startTriggerScanner();
    Serial.println("[Scanner] High-precision scanner started (esp_timer @ 2kHz)");

    // Initialize MIDI controller
    Serial.println("\n[MIDI] Initializing USB MIDI...");
    MIDIController::begin();

    // Initialize NeoPixel LEDs
    Serial.println("\n[LED] Initializing NeoPixels...");
    NeoPixelController::begin();

    // Set idle color (mismo blanco azulado para todos los pads)
    uint32_t idleColor = ((uint32_t)PAD_LED_IDLE_COLOR.r << 16) |
                         ((uint32_t)PAD_LED_IDLE_COLOR.g << 8) |
                         PAD_LED_IDLE_COLOR.b;
    for (uint8_t i = 0; i < NUM_PADS; i++) {
        NeoPixelController::setIdleColor(i, idleColor, 40);  // 40% brightness for idle
    }
    Serial.println("[LED] NeoPixels initialized with idle colors");

    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║            SISTEMA INICIADO - LISTO           ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.println();
    printHelp();
    Serial.println("\n✅ Golpea los pads para comenzar...\n");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
    // Process hit events from queue
    processHitEvents();

    // Update MIDI (process note offs)
    MIDIController::update();

    // Update LED animations
    NeoPixelController::update();

    // Handle serial commands
    handleSerialCommands();

    // Process calibration if active
    if (calibrationMode) {
        processCalibration();
    }

    delay(1);  // Yield to FreeRTOS
}

// ============================================================
// HARDWARE SETUP
// ============================================================

void setupHardware() {
    Serial.println("[Hardware] Configurando ADC...");

    // Configure ADC for all pads
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_ATTENUATION);

    Serial.printf("  Resolución: %d-bit (0-4095)\n", ADC_RESOLUTION);
    Serial.printf("  Atenuación: 11dB (0-2.45V)\n");
    Serial.printf("  Sample Rate: %d Hz\n", SCAN_RATE_HZ);
    Serial.println("\n  Mapeo de pines:");
    for (int i = 0; i < NUM_PADS; i++) {
        Serial.printf("    Pad %d (%s) -> GPIO %d\n",
                      i, PAD_NAMES[i], PAD_ADC_PINS[i]);
    }
}

// ============================================================
// HIT EVENT PROCESSING
// ============================================================

void processHitEvents() {
    HitEvent event;

    // Check if there are events in the queue
    while (xQueueReceive(hitEventQueue, &event, 0) == pdTRUE) {
        totalHitsDetected++;

        // Print hit information
        Serial.printf("🥁 HIT: %s | Velocity=%3d | Baseline=%3d | Total=%u\n",
                      PAD_NAMES[event.padId],
                      event.velocity,
                      triggerDetector.getBaseline(event.padId),
                      totalHitsDetected);

        // Send MIDI Note
        uint8_t midiNote = PAD_MIDI_NOTES[event.padId];
        MIDIController::sendNoteOn(midiNote, event.velocity);

        // Flash LED con el color asignado al pad (cian/rosa/amarillo/verde)
        CRGB color = PAD_LED_HIT_COLORS[event.padId];
        uint32_t hitColor = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
        uint8_t brightness = map(event.velocity, 0, 127, 100, 255);  // Map velocity to brightness
        NeoPixelController::flashPad(event.padId, hitColor, brightness, 300);

        // TODO: Play audio sample
    }
}

// ============================================================
// SERIAL COMMANDS
// ============================================================

void handleSerialCommands() {
    if (!Serial.available()) return;

    char cmd = Serial.read();

    switch (cmd) {
        case 's':
        case 'S':
            printStats();
            break;

        case 'd':
        case 'D':
            printDetectorState();
            break;

        case 'c':
        case 'C':
            startCalibration();
            break;

        case 'r':
        case 'R':
            triggerScanner.resetStats();
            triggerDetector.resetAll();
            totalHitsDetected = 0;
            Serial.println("✅ Sistema reseteado\n");
            break;

        case 'h':
        case 'H':
            printHelp();
            break;

        default:
            break;
    }
}

// ============================================================
// STATISTICS & STATE
// ============================================================

void printStats() {
    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║         ESTADÍSTICAS DEL SISTEMA              ║");
    Serial.println("╚═══════════════════════════════════════════════╝");

    uint32_t avgUs, maxUs, minUs;
    triggerScanner.getStats(avgUs, maxUs, minUs);

    Serial.printf("Total hits detectados: %u\n\n", totalHitsDetected);

    Serial.println("--- Scanner Performance ---");
    Serial.printf("Promedio de scan: %u µs\n", avgUs);
    Serial.printf("Máximo scan:      %u µs\n", maxUs);
    Serial.printf("Mínimo scan:      %u µs\n", minUs);
    Serial.printf("Target period:    %d µs\n", SCAN_PERIOD_US);

    if (avgUs > 0) {
        float actualRate = 1000000.0f / avgUs;
        Serial.printf("Sample rate real: %.1f Hz\n", actualRate);
    }

    Serial.println("\n--- Baselines por Pad ---");
    for (int i = 0; i < NUM_PADS; i++) {
        uint16_t baseline = triggerDetector.getBaseline(i);
        Serial.printf("%s: %d ADC (%.2fV) | Threshold: %d ADC\n",
                      PAD_NAMES[i],
                      baseline,
                      (baseline * 2.45f) / 4095.0f,
                      TRIGGER_THRESHOLD_PER_PAD[i]);
    }

    Serial.println();
}

void printDetectorState() {
    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║         ESTADO DEL DETECTOR                   ║");
    Serial.println("╚═══════════════════════════════════════════════╝\n");

    triggerDetector.printState();
    Serial.println();
}

void printHelp() {
    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║              COMANDOS DISPONIBLES             ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.println("  's' - Mostrar estadísticas completas");
    Serial.println("  'd' - Mostrar estado del detector (baselines, states)");
    Serial.println("  'c' - Calibrar thresholds (30s automático)");
    Serial.println("  'r' - Reset sistema completo");
    Serial.println("  'h' - Mostrar esta ayuda");
    Serial.println();
}

// ============================================================
// CALIBRATION MODE
// ============================================================

void startCalibration() {
    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║        MODO CALIBRACIÓN ACTIVADO (30s)        ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("📝 Instrucciones:");
    Serial.println("   Durante los próximos 30 segundos:");
    Serial.println("   1. Golpea CADA pad con diferentes intensidades");
    Serial.println("   2. Incluye golpes MUY SUAVES (mínimo deseado)");
    Serial.println("   3. Incluye golpes FUERTES (máximo esperado)");
    Serial.println("   4. Observa los baselines (ruido en reposo)");
    Serial.println();
    Serial.println("   Al finalizar, verás thresholds sugeridos");
    Serial.println("   basados en el ruido observado de TUS piezos.");
    Serial.println();
    Serial.println("⚠️  Presiona cualquier tecla para CANCELAR");
    Serial.println("\nComenzando en 3 segundos...\n");

    delay(3000);

    calibrationMode = true;
    calibrationStartTime = millis();

    // Reset calibration data
    for (int i = 0; i < NUM_PADS; i++) {
        calibrationPeaks[i] = 0;
        calibrationMins[i] = 4095;
    }

    Serial.println("🎯 CALIBRACIÓN INICIADA - Golpea todos los pads!\n");
}

void processCalibration() {
    uint32_t elapsed = millis() - calibrationStartTime;

    // Show progress every 5 seconds
    static uint32_t lastProgressTime = 0;
    if (millis() - lastProgressTime > 5000) {
        lastProgressTime = millis();
        uint32_t remaining = (30000 - elapsed) / 1000;
        Serial.printf("⏱️  %u segundos restantes...\n", remaining);
    }

    // Check for cancellation
    if (Serial.available()) {
        calibrationMode = false;
        Serial.println("\n❌ Calibración cancelada\n");
        while (Serial.available()) Serial.read();  // Clear buffer
        return;
    }

    // Check if calibration time expired
    if (elapsed > 30000) {
        calibrationMode = false;

        Serial.println("\n\n╔═══════════════════════════════════════════════╗");
        Serial.println("║         CALIBRACIÓN COMPLETADA ✅             ║");
        Serial.println("╚═══════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("📊 Thresholds sugeridos para main.cpp:");
        Serial.println("   (Basado en ruido observado + margen de 80 ADC)\n");

        Serial.println("const uint16_t TRIGGER_THRESHOLD_PER_PAD[4] = {");
        for (int i = 0; i < NUM_PADS; i++) {
            uint16_t baseline = triggerDetector.getBaseline(i);
            uint16_t suggested = baseline + 80;  // Margen de 80 ADC sobre baseline
            Serial.printf("    %3d,  // %s (baseline observado: ~%d ADC, %.2fV)\n",
                          suggested,
                          PAD_NAMES[i],
                          baseline,
                          (baseline * 2.45f) / 4095.0f);
        }
        Serial.println("};");
        Serial.println();

        Serial.println("📊 Rangos de velocity sugeridos:\n");
        Serial.println("const uint16_t VELOCITY_MIN_PEAK[4] = {");
        for (int i = 0; i < NUM_PADS; i++) {
            uint16_t minVal = (calibrationMins[i] == 4095) ? 100 : calibrationMins[i];
            Serial.printf("    %4d,  // %s (golpe suave observado)\n",
                          minVal, PAD_NAMES[i]);
        }
        Serial.println("};");
        Serial.println();

        Serial.println("const uint16_t VELOCITY_MAX_PEAK[4] = {");
        for (int i = 0; i < NUM_PADS; i++) {
            uint16_t maxVal = (calibrationPeaks[i] == 0) ? 2000 : calibrationPeaks[i];
            Serial.printf("    %4d,  // %s (golpe fuerte observado)\n",
                          maxVal, PAD_NAMES[i]);
        }
        Serial.println("};");
        Serial.println();

        Serial.println("💡 Copia estos valores a main.cpp (líneas 36-59)");
        Serial.println("   Recompila y sube para aplicar la calibración.\n");
    }
}

// ============================================================
// SAFETY CHECK (called by scanner)
// ============================================================

void checkADCSafety(uint16_t value, uint8_t padId) {
    if (value > ADC_SAFETY_LIMIT) {
        Serial.println("\n╔═══════════════════════════════════════════════╗");
        Serial.println("║  ⚠️  ALERTA CRÍTICA: VIOLACIÓN DE SEGURIDAD  ║");
        Serial.println("╚═══════════════════════════════════════════════╝");
        Serial.printf("Pad %s: ADC = %d (EXCEDE LÍMITE %d)\n",
                      PAD_NAMES[padId], value, ADC_SAFETY_LIMIT);
        Serial.println("⚠️  ACCIÓN: Verificar circuitos de protección!\n");
    }
}
