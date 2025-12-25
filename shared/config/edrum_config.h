/**
 * @file edrum_config.h
 * @brief Central configuration file for E-Drum Controller
 * @version 1.0
 * @date 2025-12-02
 *
 * This file contains all hardware pin definitions, algorithm tuning parameters,
 * MIDI mappings, and system constants for the dual-MCU E-Drum controller.
 *
 * HARDWARE WARNING:
 * Piezo sensors MUST have protection circuits (1MΩ + 1kΩ + 2x 1N4148 diodes)
 * before connecting to ESP32-S3 ADC pins. Direct connection will destroy the ADC!
 */

#pragma once

#include <Arduino.h>
#include <FastLED.h>

// ============================================================
// HARDWARE PIN DEFINITIONS - MCU #1 (Main Brain)
// ============================================================

// ADC Trigger Inputs (4 Piezo Sensors)
#define PAD0_ADC_PIN 4    // ADC1_CH3 - Kick
#define PAD1_ADC_PIN 5    // ADC1_CH4 - Snare
#define PAD2_ADC_PIN 6    // ADC1_CH5 - HiHat
#define PAD3_ADC_PIN 7    // ADC1_CH6 - Tom

extern const int PAD_ADC_PINS[4];
extern const char* PAD_NAMES[4];

// I2S Audio Output (PCM5102 DAC)
#define I2S_BCLK_PIN 16
#define I2S_LRCK_PIN 17
#define I2S_DOUT_PIN 18

// SD Card (SPI Interface)
#define SD_CS_PIN   10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN  12

// Default sample paths on SD
#define SAMPLE_PATH_KICK   "/samples/default/kick.wav"
#define SAMPLE_PATH_SNARE  "/samples/default/snare.wav"
#define SAMPLE_PATH_HIHAT  "/samples/default/hihat.wav"
#define SAMPLE_PATH_TOM    "/samples/default/tom.wav"

// --- MIDI OUTPUT (Hardware Serial 1) ---
#define MIDI_TX_PIN 9    
#define MIDI_BAUD   31250

// Rotary Encoders (ALPS EC11)
// Left Encoder
#define ENC_L_A_PIN  1
#define ENC_L_B_PIN  2
#define ENC_L_SW_PIN 42   // MTMS

// Right Encoder
#define ENC_R_A_PIN  41   // MTDI
#define ENC_R_B_PIN  40   // MTDO
#define ENC_R_SW_PIN 39   // MTCK

// Buttons
#define BTN_KIT_PIN   3
#define BTN_EDIT_PIN  8
#define BTN_MENU_PIN  14
#define BTN_CLICK_PIN 15
#define BTN_FX_PIN    38
#define BTN_SHIFT_PIN 0

// LED Outputs
#define LED_PADS_PIN      48  // WS2812B x4 (one per pad)
#define LED_ENC_DATA_PIN  47  // SK9822 Data (2 rings x 12 LEDs each)
#define LED_ENC_CLK_PIN   21  // SK9822 Clock

#define NUM_LEDS_PADS      4
#define NUM_LEDS_ENCODERS  24  // 2 rings x 12 LEDs each
#define LEDS_PER_ENCODER   12  // LEDs per encoder ring

// UART Communication to MCU #2
#define UART_TX_PIN 2    // TX hacia display (antes 43)
#define UART_RX_PIN 1    // RX desde display (antes 44)
#define UART_BAUD   921600

// ============================================================
// HARDWARE PIN DEFINITIONS - MCU #2 (Display)
// ============================================================

#ifdef MCU_DISPLAY

// TFT Display (GC9A01)
#ifndef TFT_MOSI
#define TFT_MOSI       11
#endif
#ifndef TFT_SCLK
#define TFT_SCLK       12
#endif
#ifndef TFT_CS
#define TFT_CS         10
#endif
#ifndef TFT_DC
#define TFT_DC         8
#endif
#ifndef TFT_RST
#define TFT_RST        14
#endif
#ifndef TFT_BACKLIGHT
#define TFT_BACKLIGHT  2
#endif

// Touch Controller (CST816S I2C)
#define TOUCH_SDA 6
#define TOUCH_SCL 7
#define TOUCH_INT 5
#define TOUCH_RST 13

// LED Ring (WS2812B x12)
#define LED_RING_PIN     18  // Display MCU drives 12-pixel NeoPixel ring
#define NUM_LEDS_RING    12

// UART from MCU #1
#define UART_RX_DISPLAY  16  // RX (display) connected to main TX (GPIO 2)
#define UART_TX_DISPLAY  17  // TX (display) connected to main RX (GPIO 1)

#endif // MCU_DISPLAY

// ============================================================
// TRIGGER DETECTION ALGORITHM PARAMETERS
// ============================================================
// NOTE: Most trigger parameters are now configured per-pad via PadConfigManager
// See: shared/config/pad_config.h for dynamic configuration system
// The following are only hardware-level constants and fallback values

// ADC Configuration
#define ADC_RESOLUTION 12               // 12-bit ADC (0-4095)
#define ADC_MAX_VALUE 4095
#define ADC_ATTENUATION ADC_11db        // 0-2.45V usable range

// Scan Rate (hardware timer configuration)
#define SCAN_PERIOD_US 500              // 500µs = 2kHz scan rate
#define SCAN_RATE_HZ 2000

// Baseline Tracking (for DC offset compensation)
#define BASELINE_UPDATE_WEIGHT 1024     // Exponential moving average weight (1/1024)
#define BASELINE_INITIAL_VALUE 150      // Initial baseline value
#define MIN_BASELINE_VALUE 50           // Minimum baseline value to prevent collapse to 0

// Legacy trigger parameters (for backward compatibility with old detector code)
#define TRIGGER_SCAN_TIME_US 2000       // Peak detection window (2ms)
#define TRIGGER_MASK_TIME_US 10000      // Retrigger suppression (10ms)
#define TRIGGER_RETRIGGER_THRESHOLD 30  // Signal must drop below this to re-arm
#define TRIGGER_CROSSTALK_WINDOW_US 50000  // Crosstalk check window (50ms)
#define TRIGGER_CROSSTALK_RATIO 0.7f    // Velocity ratio for crosstalk rejection
#define VELOCITY_CURVE_EXPONENT 0.5f    // Velocity curve exponent (sqrt)

// DEPRECATED: Legacy arrays below are kept for backward compatibility only
// New code should use PadConfigManager::getConfig(padId) instead
extern const uint16_t TRIGGER_THRESHOLD_PER_PAD[4];  // Use cfg.threshold
extern const uint16_t VELOCITY_MIN_PEAK[4];          // Use cfg.velocityMin
extern const uint16_t VELOCITY_MAX_PEAK[4];          // Use cfg.velocityMax
extern const uint8_t PAD_MIDI_NOTES[4];              // Use cfg.midiNote
extern const CRGB PAD_LED_COLORS[4];                 // Use cfg.ledColorHit

// ============================================================
// MIDI CONFIGURATION
// ============================================================

// Default MIDI channel (9 = channel 10 in 1-based, standard drum channel)
#define DEFAULT_MIDI_CHANNEL 9

// MIDI velocity range (1-127, 0 reserved for note-off)
#define MIDI_VELOCITY_MIN 1
#define MIDI_VELOCITY_MAX 127

// Auto Note-Off timing (milliseconds)
#define MIDI_NOTE_OFF_DELAY_MS 100

// ============================================================
// LED ANIMATION CONFIGURATION
// ============================================================

// Pad LED Idle Brightness (0-255)
#define PAD_LED_IDLE_BRIGHTNESS 30

// Hit Flash Effect Timing
#define LED_FLASH_DURATION_MS 10      // White flash duration
#define LED_DECAY_DURATION_MS 150     // Exponential decay back to idle color

// Encoder Ring Animation
#define ENCODER_LED_IDLE_COLOR CRGB::Cyan
#define ENCODER_LED_BREATHING_PERIOD_MS 2000
#define ENCODER_LED_BRIGHTNESS_MIN 0.1f    // 10%
#define ENCODER_LED_BRIGHTNESS_MAX 0.4f    // 40%

// LED Update Rate
#define LED_UPDATE_FPS 60
#define LED_UPDATE_PERIOD_MS (1000 / LED_UPDATE_FPS)

// ============================================================
// FREERTOS TASK CONFIGURATION
// ============================================================

// Task Stack Sizes (bytes)
#define TASK_STACK_TRIGGER_SCAN  4096
#define TASK_STACK_MIDI_OUTPUT   4096
#define TASK_STACK_LED_ANIMATION 4096
#define TASK_STACK_UART_COMM     4096
#define TASK_STACK_BUTTON_READER 2048

// Task Priorities (0-24, higher = more priority)
#define TASK_PRIORITY_TRIGGER_SCAN  24  // Highest - real-time trigger detection
#define TASK_PRIORITY_UART_COMM     15  // High - communication
#define TASK_PRIORITY_MIDI_OUTPUT   10  // Medium - MIDI output
#define TASK_PRIORITY_LED_ANIMATION 5   // Low - visual feedback
#define TASK_PRIORITY_BUTTON_READER 5   // Low - user input

// Core Assignment
#define TASK_CORE_TRIGGER_SCAN   0  // Core 0: Real-time trigger scanning
#define TASK_CORE_MIDI_OUTPUT    1  // Core 1: MIDI and communication
#define TASK_CORE_LED_ANIMATION  1  // Core 1: LED animations
#define TASK_CORE_UART_COMM      1  // Core 1: UART communication

// ============================================================
// QUEUE SIZES
// ============================================================

#define QUEUE_SIZE_HIT_EVENTS  16   // Buffer for hit events (trigger → MIDI)
#define QUEUE_SIZE_UART_TX     32   // UART transmit buffer
#define QUEUE_SIZE_UART_RX     32   // UART receive buffer

// ============================================================
// DEBUG CONFIGURATION
// ============================================================

// Enable/disable debug output (comment out for production)
#define DEBUG_SERIAL_ENABLE
#define DEBUG_BAUD_RATE 115200

// Debug output levels
// #define DEBUG_TRIGGER_RAW        // Print raw ADC values
// #define DEBUG_TRIGGER_EVENTS     // Print hit events with velocity
// #define DEBUG_TRIGGER_TIMING     // Print timing metrics
// #define DEBUG_MIDI_MESSAGES      // Print MIDI messages
// #define DEBUG_UART_PROTOCOL      // Print UART frames

// ============================================================
// SYSTEM CONSTANTS
// ============================================================

#define NUM_PADS 4
#define NUM_ENCODERS 2
#define NUM_BUTTONS 6

// Button IDs
enum ButtonID {
    BTN_KIT = 0,
    BTN_EDIT = 1,
    BTN_MENU = 2,
    BTN_CLICK = 3,
    BTN_FX = 4,
    BTN_SHIFT = 5
};

// Encoder IDs
enum EncoderID {
    ENC_LEFT = 0,
    ENC_RIGHT = 1
};

// Button States
enum ButtonState {
    STATE_RELEASED = 0,
    STATE_PRESSED = 1,
    STATE_LONG_PRESS = 2,
    STATE_DOUBLE_CLICK = 3
};

// ============================================================
// SAFETY LIMITS
// ============================================================

// Maximum safe ADC value (if exceeded, protection circuit may have failed)
#define ADC_SAFETY_LIMIT 3800

// Temperature monitoring (if available)
#define TEMP_WARNING_CELSIUS 70
#define TEMP_SHUTDOWN_CELSIUS 85

// ============================================================
// VERSION INFORMATION
// ============================================================

#define FIRMWARE_VERSION "0.0.8"
#define FIRMWARE_BUILD_DATE __DATE__
#define FIRMWARE_BUILD_TIME __TIME__

// ============================================================
// MACROS
// ============================================================

// Map ADC value (0-4095) to voltage (0-2.45V with 11dB attenuation)
#define ADC_TO_VOLTAGE(adc) ((adc) * 2.45f / 4095.0f)

// Clamp value to range
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// Check if value is in range
#define IN_RANGE(x, min, max) ((x) >= (min) && (x) <= (max))
