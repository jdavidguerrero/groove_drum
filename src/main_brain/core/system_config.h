/**
 * @file system_config.h
 * @brief System hardware initialization and configuration
 * @version 1.0
 * @date 2025-12-02
 *
 * This module handles all hardware peripheral initialization:
 * - ADC configuration for piezo triggers
 * - GPIO setup for buttons, encoders, LEDs
 * - UART initialization for MIDI and MCU#2 communication
 * - SPI setup for SD card
 * - I2S configuration for audio output (future)
 */

#pragma once

#include <Arduino.h>
#include <edrum_config.h>

// ============================================================
// SYSTEM INITIALIZATION
// ============================================================

/**
 * @brief Initialize all hardware peripherals
 * @return true if initialization successful, false otherwise
 */
bool systemInit();

/**
 * @brief Configure ADC for piezo trigger reading
 * Sets up 12-bit resolution, 11dB attenuation, and calibration
 * @return true if successful
 */
bool configureADC();

/**
 * @brief Configure all GPIO pins (buttons, encoders, LEDs)
 * Sets pin modes and initial states
 * @return true if successful
 */
bool configurePins();

/**
 * @brief Configure UART ports
 * - UART1 (Serial1): MIDI output at 31250 baud
 * - UART2 (Serial2): Communication to MCU#2 at 921600 baud
 * @return true if successful
 */
bool configureUART();

/**
 * @brief Configure SPI for SD card
 * Sets up SPI bus and CS pin
 * @return true if successful
 */
bool configureSPI();

/**
 * @brief Configure I2S for audio output (PCM5102 DAC)
 * This is for future audio playback functionality
 * @return true if successful
 */
bool configureI2S();

/**
 * @brief Test ADC by reading all pad channels
 * Used during initialization to verify hardware
 * @return true if ADC readings are within safe range
 */
bool testADC();

/**
 * @brief Print system configuration to serial
 * Useful for debugging and verification
 */
void printSystemInfo();

/**
 * @brief Check for ADC safety violations
 * Monitors for values exceeding safe limits (indicates protection circuit failure)
 * @param adcValue Raw ADC reading
 * @param padId Pad ID for error reporting
 * @return true if value is safe, false if exceeded safety limit
 */
// Note: public checkADCSafety is implemented in main.cpp for the scanner.
// Internal ADC checks inside system_config use a private helper.
