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
#include "pad_config.h"
#include "ui/neopixel_controller.h"
#include "ui/encoder_handler.h"
#include "ui/button_handler.h"
#include "ui/menu_system.h"
#include "output/midi_controller.h"
#include "output/audio_engine.h"
#include "output/audio_samples.h"
#include "core/event_dispatcher.h"
#include "communication/uart_protocol.h"

// ============================================================
// CONFIG ARRAY DEFINITIONS (from edrum_config.h)
// ============================================================

const int PAD_ADC_PINS[4] = {PAD0_ADC_PIN, PAD1_ADC_PIN, PAD2_ADC_PIN, PAD3_ADC_PIN};
const char* PAD_NAMES[4] = {"PAD1", "PAD2", "PAD3", "PAD4"};

// Threshold por pad - Ajustar según ruido observado de cada piezo
// Valores más altos = menos sensibilidad, menos falsos triggers
const uint16_t TRIGGER_THRESHOLD_PER_PAD[4] = {
    350,  // Pad 0 (Kick) - aumentado para evitar ruido
    350,  // Pad 1 (Snare) - aumentado
    450,  // Pad 2 (HiHat) - MUY aumentado por ruido excesivo
    350   // Pad 3 (Tom) - aumentado
};

// Rangos de velocity - Calibrar con el comando 'c'
const uint16_t VELOCITY_MIN_PEAK[4] = {
    200,  // Kick - aumentado
    200,  // Snare - aumentado
    250,  // HiHat - aumentado significativamente
    200   // Tom - aumentado
};

const uint16_t VELOCITY_MAX_PEAK[4] = {
    3500, 3500, 3000, 3500
};

const uint8_t PAD_MIDI_NOTES[4] = {36, 38, 42, 48};
const CRGB PAD_LED_IDLE_COLOR = CRGB(10, 10, 10);
const CRGB PAD_LED_HIT_COLORS[4] = {
    CRGB(0, 255, 255),
    CRGB(255, 50, 150),
    CRGB(255, 255, 0),
    CRGB(0, 255, 100)
};

// ============================================================
// GLOBAL VARIABLES
// ============================================================

QueueHandle_t hitEventQueue;
bool calibrationMode = false;
uint32_t calibrationStartTime = 0;
uint16_t calibrationPeaks[4] = {0, 0, 0, 0};
uint16_t calibrationMins[4] = {4095, 4095, 4095, 4095};
uint32_t totalHitsDetected = 0;
bool audioEngineInitialized = false;
bool samplesLoaded = false;
uint32_t lastStatusBroadcastMs = 0;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void setupHardware();
void processHitEvents();
void processUIInputs();
void handleSerialCommands();
void printHelp();
void printStats();
void printDetectorState();
void startCalibration();
void processCalibration();
void checkADCSafety(uint16_t value, uint8_t padId);
void queueSamplePlayback(const char* name, uint8_t velocity = 120);
void queuePadSample(uint8_t padId, uint8_t velocity);

// ============================================================
// SETUP
// ============================================================

void setup() {
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

    // =========================================================
    // CRITICAL: Stabilize ALL GPIO pins BEFORE SD card init
    // GPIO21 and GPIO47 (SK9822 LEDs) cause SPI interference
    // when panel connector is attached but LEDs not connected
    // =========================================================
    Serial.println("[GPIO] Stabilizing pins before SD init...");

    // SK9822 LED pins - set as OUTPUT LOW to prevent interference
    pinMode(LED_ENC_CLK_PIN, OUTPUT);   // GPIO21
    digitalWrite(LED_ENC_CLK_PIN, LOW);
    pinMode(LED_ENC_DATA_PIN, OUTPUT);  // GPIO47
    digitalWrite(LED_ENC_DATA_PIN, LOW);

    // NeoPixel pin
    pinMode(LED_PADS_PIN, OUTPUT);      // GPIO48
    digitalWrite(LED_PADS_PIN, LOW);

    delay(50);
    Serial.println("[GPIO] Pins stabilized");

    PadConfigManager::init();

    // Initialize SD card FIRST (before I2S which also uses DMA)
    Serial.println("[SD] Loading samples from SD card...");
    size_t loadedSamples = SampleManager::beginAndLoadDefaults();
    if (loadedSamples > 0) {
        samplesLoaded = true;
        Serial.printf("[SD] Loaded %u samples\n", (unsigned)loadedSamples);
    } else {
        Serial.println("[SD] No samples loaded - check SD card");
    }

    Serial.println("[UART] Initializing display link...");
    UARTProtocol::begin(Serial2, UART_BAUD, UART_RX_PIN, UART_TX_PIN);

    Serial.println("[MIDI] Initializing USB MIDI...");
    MIDIController::begin();

    Serial.println("[AUDIO] Initializing I2S audio engine...");
    audioEngineInitialized = AudioEngine::begin();
    if (!audioEngineInitialized) {
        Serial.println("[AUDIO] Audio engine init failed");
    }

    Serial.println("[Dispatcher] Initializing subsystems...");
    EventDispatcher::begin();

    setupHardware();

    hitEventQueue = xQueueCreate(QUEUE_SIZE_HIT_EVENTS, sizeof(HitEvent));
    if (!hitEventQueue) {
        Serial.println("[ERROR] Failed to create hit event queue!");
        while (1) delay(1000);
    }
    Serial.println("[Queue] Hit event queue created");

    Serial.println("\n[System] Initializing trigger detection...");
    triggerScanner.begin(hitEventQueue);
    startTriggerScanner();
    Serial.println("[Scanner] High-precision scanner started (esp_timer @ 2kHz)");

    Serial.println("\n[LED] Initializing NeoPixels...");
    NeoPixelController::begin();

    uint32_t idleColor = ((uint32_t)PAD_LED_IDLE_COLOR.r << 16) |
                         ((uint32_t)PAD_LED_IDLE_COLOR.g << 8) |
                         PAD_LED_IDLE_COLOR.b;
    for (uint8_t i = 0; i < NUM_PADS; i++) {
        NeoPixelController::setIdleColor(i, idleColor, 40);
    }
    Serial.println("[LED] NeoPixels initialized with idle colors");

    // Initialize UI inputs (encoders and buttons)
    Serial.println("\n[UI] Initializing encoders and buttons...");
    EncoderHandler::begin();
    ButtonHandler::begin();
    Serial.println("[UI] Encoders and buttons initialized");

    // Initialize menu system
    Serial.println("[UI] Initializing menu system...");
    MenuSystem::begin();
    Serial.println("[UI] Menu system initialized");

    Serial.println("\n╔═══════════════════════════════════════════════╗");
    Serial.println("║            SISTEMA INICIADO - LISTO           ║");
    Serial.println("╚═══════════════════════════════════════════════╝");
    Serial.println();
    printHelp();
    Serial.println("\n✅ Golpea los pads para comenzar...\n");
    Serial.println("📋 Presiona BTN_MENU (GPIO14) para configurar pads\n");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
    UARTProtocol::processIncoming();
    processHitEvents();
    processUIInputs();
    MenuSystem::update();  // Update menu state machine
    EventDispatcher::processAudio();  // Process queued audio samples
    MIDIController::update();
    NeoPixelController::update();
    handleSerialCommands();

    if (calibrationMode) {
        processCalibration();
    }

    if (millis() - lastStatusBroadcastMs > 1000) {
        UARTProtocol::sendSystemStatus();
        lastStatusBroadcastMs = millis();
    }

    delay(1);
}

// ============================================================
// HARDWARE SETUP
// ============================================================

void setupHardware() {
    Serial.println("[Hardware] Configurando ADC...");

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
// HIT EVENT PROCESSING (WITH CROSSTALK SUPPRESSION)
// ============================================================

#define CROSSTALK_WINDOW_MS 4  // Time window to group simultaneous hits
#define MIN_INTER_HIT_TIME_MS 40 // Minimum time between hits on SAME pad (Debounce)

struct PendingHit {
    HitEvent event;
    bool valid;
};

// Simple buffer for pending hits (max hits that can physically happen in 4ms)
#define MAX_PENDING_HITS 8
PendingHit pendingHits[MAX_PENDING_HITS];
uint8_t pendingCount = 0;
uint32_t groupWindowStartTime = 0;
bool windowActive = false;
uint32_t lastHitTimeMs[NUM_PADS] = {0, 0, 0, 0}; // Debounce timers

void flushPendingHits() {
    if (pendingCount == 0) return;

    // 1. Find the strongest hit (Master)
    uint8_t maxVelocity = 0;
    int strongestIdx = -1;

    for (uint8_t i = 0; i < pendingCount; i++) {
        if (pendingHits[i].event.velocity > maxVelocity) {
            maxVelocity = pendingHits[i].event.velocity;
            strongestIdx = i;
        }
    }

    // 2. Suppress weaker hits (Slaves) based on ratio
    //    Using PadConfigManager to get specific crosstalk settings if possible,
    //    otherwise falling back to a global safe ratio.
    if (strongestIdx != -1) {
        for (uint8_t i = 0; i < pendingCount; i++) {
            if (i == strongestIdx) continue;

            // Get config for the victim pad
            PadConfig& cfg = PadConfigManager::getConfig(pendingHits[i].event.padId);
            
            if (cfg.crosstalkEnabled) {
                // If victim's velocity is less than X% of master, KILL IT.
                // Standard ratio is ~0.7 (70%)
                float ratio = cfg.crosstalkRatio > 0 ? cfg.crosstalkRatio : 0.6f; 
                
                // Aggressive check: if master is VERY strong (>100), harder to survive
                if (maxVelocity > 100) ratio += 0.1f; 

                if (pendingHits[i].event.velocity < (maxVelocity * ratio)) {
                    pendingHits[i].valid = false;
                    Serial.printf("🚫 X-TALK: Pad %s (Vel %d) suppressed by %s (Vel %d)\n",
                        PAD_NAMES[pendingHits[i].event.padId],
                        pendingHits[i].event.velocity,
                        PAD_NAMES[pendingHits[strongestIdx].event.padId],
                        maxVelocity);
                }
            }
        }
    }

    // 3. Process Survivors with Debounce
    uint32_t now = millis();
    for (uint8_t i = 0; i < pendingCount; i++) {
        if (!pendingHits[i].valid) continue;

        HitEvent& event = pendingHits[i].event;
        
        // DEBOUNCE CHECK: Ignore if hit too recently
        if (now - lastHitTimeMs[event.padId] < MIN_INTER_HIT_TIME_MS) {
            Serial.printf("🛡️ DEBOUNCE: Ignored rapid retrigger on %s (%d ms)\n", 
                          PAD_NAMES[event.padId], now - lastHitTimeMs[event.padId]);
            continue;
        }
        lastHitTimeMs[event.padId] = now;

        totalHitsDetected++;

        uint8_t velocity = CLAMP(event.velocity, 1, 127);

        Serial.printf("🥁 HIT: %s | Velocity=%3d | Baseline=%3d | Total=%u\n",
                      PAD_NAMES[event.padId],
                      velocity,
                      triggerDetector.getBaseline(event.padId),
                      totalHitsDetected);

        uint8_t midiNote = PAD_MIDI_NOTES[event.padId];
        MIDIController::sendNoteOn(midiNote, velocity);

        CRGB color = PAD_LED_HIT_COLORS[event.padId];
        uint32_t hitColor = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
        uint8_t brightness = map(velocity, 0, 127, 100, 255);
        NeoPixelController::flashPad(event.padId, hitColor, brightness, 300);

        queuePadSample(event.padId, velocity);

        UARTProtocol::sendHitEvent(event.padId, velocity, event.timestamp, event.peakValue);
        const PadState& padState = triggerDetector.getPadState(event.padId);
        UARTProtocol::sendPadState(
            event.padId,
            static_cast<uint8_t>(padState.state),
            padState.peakValue,
            padState.baselineValue,
            event.peakValue);
    }

    // Reset buffer
    pendingCount = 0;
    windowActive = false;
}

void processHitEvents() {
    HitEvent event;

    // 1. Drain Queue into Buffer
    while (xQueueReceive(hitEventQueue, &event, 0) == pdTRUE) {
        // If window inactive, start it
        if (!windowActive) {
            windowActive = true;
            groupWindowStartTime = millis();
        }

        // Add to buffer if space exists
        if (pendingCount < MAX_PENDING_HITS) {
            pendingHits[pendingCount].event = event;
            pendingHits[pendingCount].valid = true;
            pendingCount++;
        }
    }

    // 2. Check Window Expiry
    if (windowActive) {
        if (millis() - groupWindowStartTime >= CROSSTALK_WINDOW_MS) {
            flushPendingHits();
        }
    }
}

// ============================================================
// UI INPUT PROCESSING (Encoders & Buttons)
// ============================================================
// Control mapping:
//   BTN_MENU (GPIO14): Enter/Exit menu
//   Encoder Left: Navigate / Adjust values
//   Encoder Left Switch (GPIO9): Select/Confirm
//   BTN_EDIT (GPIO8): Quick switch between pads
//   BTN_FX (GPIO38): Save configuration
//   BTN_CLICK (GPIO15): Cancel/Back

void processUIInputs() {
    // Update encoder and button states
    EncoderHandler::update();
    ButtonHandler::update();

    // ========== LEFT ENCODER (Main navigation) ==========
    EncoderHandler::EncoderEvent encLeftEvent = EncoderHandler::pollEvent(ENC_LEFT);
    if (encLeftEvent != EncoderHandler::EVENT_NONE) {
        switch (encLeftEvent) {
            case EncoderHandler::EVENT_ROTATED_CW:
                MenuSystem::onEncoderRotate(+1);
                break;
            case EncoderHandler::EVENT_ROTATED_CCW:
                MenuSystem::onEncoderRotate(-1);
                break;
            case EncoderHandler::EVENT_SWITCH_PRESSED:
                MenuSystem::onEncoderPress();
                break;
            case EncoderHandler::EVENT_SWITCH_LONG_PRESS:
                // Long press on encoder = quick save
                if (MenuSystem::isActive()) {
                    MenuSystem::onButtonFX();
                }
                break;
            default:
                break;
        }
    }

    // ========== RIGHT ENCODER (Secondary - volume/etc) ==========
    EncoderHandler::EncoderEvent encRightEvent = EncoderHandler::pollEvent(ENC_RIGHT);
    if (encRightEvent != EncoderHandler::EVENT_NONE) {
        // Right encoder for future volume control
        // Switch is disabled (GPIO45)
        switch (encRightEvent) {
            case EncoderHandler::EVENT_ROTATED_CW:
                // TODO: Master volume up
                break;
            case EncoderHandler::EVENT_ROTATED_CCW:
                // TODO: Master volume down
                break;
            default:
                break;
        }
    }

    // ========== BTN_MENU (GPIO14) - Enter/Exit Menu ==========
    ButtonHandler::ButtonEvent menuEvent = ButtonHandler::pollEvent(BTN_MENU);
    if (menuEvent == ButtonHandler::EVENT_CLICK) {
        MenuSystem::onButtonMenu();
    } else if (menuEvent == ButtonHandler::EVENT_LONG_PRESS) {
        // Long press = force exit without saving
        if (MenuSystem::isActive()) {
            Serial.println("[MENU] Force exit");
            MenuSystem::onButtonClick();  // Back
            MenuSystem::onButtonClick();  // Exit
        }
    }

    // ========== BTN_EDIT (GPIO8) - Quick Pad Switch ==========
    ButtonHandler::ButtonEvent editEvent = ButtonHandler::pollEvent(BTN_EDIT);
    if (editEvent == ButtonHandler::EVENT_CLICK) {
        MenuSystem::onButtonEdit();
    } else if (editEvent == ButtonHandler::EVENT_LONG_PRESS) {
        // Long press = enter menu directly to config
        if (!MenuSystem::isActive()) {
            MenuSystem::onButtonMenu();
        }
    }

    // ========== BTN_CLICK (GPIO15) - Cancel/Back ==========
    ButtonHandler::ButtonEvent clickEvent = ButtonHandler::pollEvent(BTN_CLICK);
    if (clickEvent == ButtonHandler::EVENT_CLICK) {
        MenuSystem::onButtonClick();
    }

    // ========== BTN_FX (GPIO38) - Save Configuration ==========
    ButtonHandler::ButtonEvent fxEvent = ButtonHandler::pollEvent(BTN_FX);
    if (fxEvent == ButtonHandler::EVENT_CLICK) {
        MenuSystem::onButtonFX();
    } else if (fxEvent == ButtonHandler::EVENT_LONG_PRESS) {
        // Long press = save and exit
        if (MenuSystem::isActive()) {
            MenuSystem::onButtonFX();
            MenuSystem::onButtonMenu();
        }
    }
}

// ============================================================
// SERIAL COMMANDS
// ============================================================

void handleSerialCommands() {
    if (!Serial.available()) return;

    char cmd = Serial.read();

    switch (cmd) {
        case 's': case 'S': printStats(); break;
        case 'd': case 'D': printDetectorState(); break;
        case 'c': case 'C': startCalibration(); break;
        case 'r': case 'R':
            triggerScanner.resetStats();
            triggerDetector.resetAll();
            totalHitsDetected = 0;
            Serial.println("✅ Sistema reseteado\n");
            break;
        case 'a': case 'A':
            queueSamplePlayback(SAMPLE_PATH_KICK, 120);
            break;
        case '1':
            queueSamplePlayback(SAMPLE_PATH_SNARE, 120);
            break;
        case '2':
            queueSamplePlayback(SAMPLE_PATH_HIHAT, 120);
            break;
        case '3':
            queueSamplePlayback(SAMPLE_PATH_TOM, 120);
            break;
        case 'h': case 'H': printHelp(); break;
        default: break;
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

    for (int i = 0; i < NUM_PADS; i++) {
        calibrationPeaks[i] = 0;
        calibrationMins[i] = 4095;
    }

    Serial.println("🎯 CALIBRACIÓN INICIADA - Golpea todos los pads!\n");
}

void processCalibration() {
    uint32_t elapsed = millis() - calibrationStartTime;

    static uint32_t lastProgressTime = 0;
    if (millis() - lastProgressTime > 5000) {
        lastProgressTime = millis();
        uint32_t remaining = (30000 - elapsed) / 1000;
        Serial.printf("⏱️  %u segundos restantes...\n", remaining);
    }

    if (Serial.available()) {
        calibrationMode = false;
        Serial.println("\n❌ Calibración cancelada\n");
        while (Serial.available()) Serial.read();
        return;
    }

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
            uint16_t suggested = baseline + 80;
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
// SAFETY CHECK
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

void queueSamplePlayback(const char* name, uint8_t velocity) {
    if (!audioEngineInitialized || !samplesLoaded) {
        Serial.println("[AUDIO] Motor o samples no inicializados");
        return;
    }

    AudioRequest req = {};
    if (name) {
        strncpy(req.sampleName, name, sizeof(req.sampleName) - 1);
    }
    req.velocity = velocity;
    req.volume = 127;
    req.pitch = 0;
    EventDispatcher::dispatchAudio(req);
}

void queuePadSample(uint8_t padId, uint8_t velocity) {
    PadConfig& cfg = PadConfigManager::getConfig(padId % NUM_PADS);
    queueSamplePlayback(cfg.sampleName, velocity);
}
