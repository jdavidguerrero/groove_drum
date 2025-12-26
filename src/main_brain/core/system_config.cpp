/**
 * @file system_config.cpp
 * @brief Implementation of system hardware initialization
 * @version 1.0
 * @date 2025-12-02
 */

#include "system_config.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>

// ADC calibration structure
static esp_adc_cal_characteristics_t adc_chars;

// ============================================================
// SYSTEM INITIALIZATION
// ============================================================

bool systemInit() {
    Serial.begin(DEBUG_BAUD_RATE);
    delay(100);

    Serial.println("\n========================================");
    Serial.println("E-Drum Controller - System Initialization");
    Serial.println("========================================");

    // Initialize hardware peripherals
    if (!configureADC()) {
        Serial.println("[ERROR] ADC configuration failed!");
        return false;
    }
    Serial.println("[OK] ADC configured");

    if (!configurePins()) {
        Serial.println("[ERROR] GPIO configuration failed!");
        return false;
    }
    Serial.println("[OK] GPIO pins configured");

    if (!configureUART()) {
        Serial.println("[ERROR] UART configuration failed!");
        return false;
    }
    Serial.println("[OK] UART configured");

    if (!configureSPI()) {
        Serial.println("[ERROR] SPI configuration failed!");
        return false;
    }
    Serial.println("[OK] SPI configured");

    // Test ADC readings
    if (!testADC()) {
        Serial.println("[WARNING] ADC test shows potential issues");
        Serial.println("[WARNING] Check piezo protection circuits!");
    } else {
        Serial.println("[OK] ADC test passed");
    }

    printSystemInfo();

    Serial.println("========================================");
    Serial.println("System initialization complete");
    Serial.println("========================================\n");

    return true;
}

// ============================================================
// ADC CONFIGURATION
// ============================================================

bool configureADC() {
    // Set ADC resolution to 12-bit (0-4095)
    analogReadResolution(ADC_RESOLUTION);

    // Set attenuation to 11dB (0-2.45V usable range)
    // Note: ESP32-S3 ADC is non-linear at extremes, stay in 200-3500 range
    analogSetAttenuation(ADC_ATTENUATION);

    // Configure ADC calibration (compensates for chip-to-chip variation)
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,  // Default Vref in mV
        &adc_chars
    );

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("[INFO] ADC calibration: eFuse Two Point");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("[INFO] ADC calibration: eFuse Vref");
    } else {
        Serial.println("[INFO] ADC calibration: Default");
    }

    return true;
}

// ============================================================
// GPIO CONFIGURATION
// ============================================================

bool configurePins() {
    // Configure ADC pins (analog input - already set by ADC config)
    // No additional configuration needed for ADC pins

    // Configure button pins (input with internal pull-up)
    pinMode(BTN_KIT_PIN, INPUT_PULLUP);
    pinMode(BTN_EDIT_PIN, INPUT_PULLUP);
    pinMode(BTN_MENU_PIN, INPUT_PULLUP);
    pinMode(BTN_CLICK_PIN, INPUT_PULLUP);
    pinMode(BTN_FX_PIN, INPUT_PULLUP);
    pinMode(BTN_SHIFT_PIN, INPUT_PULLUP);

    // Configure encoder pins (input with internal pull-up)
    pinMode(ENC_L_A_PIN, INPUT_PULLUP);
    pinMode(ENC_L_B_PIN, INPUT_PULLUP);
    pinMode(ENC_L_SW_PIN, INPUT_PULLUP);
    pinMode(ENC_R_A_PIN, INPUT_PULLUP);
    pinMode(ENC_R_B_PIN, INPUT_PULLUP);
    pinMode(ENC_R_SW_PIN, INPUT_PULLUP);

    // Configure LED pins (output)
    pinMode(LED_PADS_PIN, OUTPUT);
    pinMode(LED_ENC_DATA_PIN, OUTPUT);
    pinMode(LED_ENC_CLK_PIN, OUTPUT);

    // Configure SD card CS pin (output, high)
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    return true;
}

// ============================================================
// UART CONFIGURATION
// ============================================================

bool configureUART() {
    // MIDI is handled via USB (TinyUSB), no hardware UART needed for MIDI

    // UART2 (Serial2): Communication to MCU#2 at 921600 baud
    // TX on GPIO 2, RX on GPIO 1
    Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    return true;
}

// ============================================================
// SPI CONFIGURATION
// ============================================================

bool configureSPI() {
    // SPI will be initialized by SD library when needed
    // Just ensure CS pin is high (deselected)
    digitalWrite(SD_CS_PIN, HIGH);

    return true;
}

// ============================================================
// I2S CONFIGURATION (FUTURE)
// ============================================================

bool configureI2S() {
    // I2S configuration will be implemented in audio_engine module
    // Placeholder for now
    return true;
}

// ============================================================
// ADC TESTING
// ============================================================

bool testADC() {
    Serial.println("\n--- ADC Test ---");
    bool allSafe = true;

    for (int pad = 0; pad < NUM_PADS; pad++) {
        uint16_t rawValue = analogRead(PAD_ADC_PINS[pad]);
        float voltage = ADC_TO_VOLTAGE(rawValue);

        Serial.printf("Pad %d (%s): Raw=%d, Voltage=%.3fV",
                      pad, PAD_NAMES[pad], rawValue, voltage);

        if (rawValue > ADC_SAFETY_LIMIT) {
            Serial.print(" [WARNING: EXCEEDS SAFETY LIMIT!]");
            allSafe = false;
        } else if (rawValue > 100) {
            Serial.print(" [Note: Non-zero at rest]");
        }

        Serial.println();
        delay(10);
    }

    Serial.println("----------------\n");
    return allSafe;
}

// ============================================================
// SYSTEM INFO
// ============================================================

void printSystemInfo() {
    Serial.println("\n--- System Configuration ---");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("Build Date: %s %s\n", FIRMWARE_BUILD_DATE, FIRMWARE_BUILD_TIME);
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM Size: %d bytes\n", ESP.getPsramSize());

    Serial.println("\n--- Pin Configuration ---");
    Serial.println("Trigger Pads:");
    for (int i = 0; i < NUM_PADS; i++) {
        Serial.printf("  Pad %d (%s): GPIO %d\n", i, PAD_NAMES[i], PAD_ADC_PINS[i]);
    }

    Serial.println("\nButtons:");
    Serial.printf("  KIT:   GPIO %d\n", BTN_KIT_PIN);
    Serial.printf("  EDIT:  GPIO %d\n", BTN_EDIT_PIN);
    Serial.printf("  MENU:  GPIO %d\n", BTN_MENU_PIN);
    Serial.printf("  CLICK: GPIO %d\n", BTN_CLICK_PIN);
    Serial.printf("  FX:    GPIO %d\n", BTN_FX_PIN);
    Serial.printf("  SHIFT: GPIO %d\n", BTN_SHIFT_PIN);

    Serial.println("\nEncoders:");
    Serial.printf("  Left:  A=%d, B=%d, SW=%d\n", ENC_L_A_PIN, ENC_L_B_PIN, ENC_L_SW_PIN);
    Serial.printf("  Right: A=%d, B=%d, SW=%d\n", ENC_R_A_PIN, ENC_R_B_PIN, ENC_R_SW_PIN);

    Serial.println("\nLEDs:");
    Serial.printf("  Pads Data:    GPIO %d\n", LED_PADS_PIN);
    Serial.printf("  Encoders Data: GPIO %d\n", LED_ENC_DATA_PIN);
    Serial.printf("  Encoders Clock: GPIO %d\n", LED_ENC_CLK_PIN);

    Serial.println("\nCommunication:");
    Serial.println("  MIDI: USB (TinyUSB)");
    Serial.printf("  UART TX:  GPIO %d @ %d baud\n", UART_TX_PIN, UART_BAUD);
    Serial.printf("  UART RX:  GPIO %d\n", UART_RX_PIN);

    Serial.println("\n--- Algorithm Parameters ---");
    Serial.printf("Scan Rate: %d Hz (%d µs period)\n", SCAN_RATE_HZ, SCAN_PERIOD_US);
    Serial.printf("Scan Time: %d µs\n", TRIGGER_SCAN_TIME_US);
    Serial.printf("Mask Time: %d µs\n", TRIGGER_MASK_TIME_US);
    Serial.printf("Crosstalk Window: %d µs\n", TRIGGER_CROSSTALK_WINDOW_US);
    Serial.printf("Velocity Curve: %.2f\n", VELOCITY_CURVE_EXPONENT);

    Serial.println("----------------------------\n");
}
