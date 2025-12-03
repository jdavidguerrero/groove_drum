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

const int PAD_ADC_PINS[4] = {PAD0_ADC_PIN, PAD1_ADC_PIN, PAD2_ADC_PIN, PAD3_ADC_PIN};
const char* PAD_NAMES[4] = {"Kick", "Snare", "HiHat", "Tom"};

// I2S Audio Output (PCM5102 DAC)
#define I2S_BCLK_PIN 26
#define I2S_LRCK_PIN 25
#define I2S_DOUT_PIN 27

// SD Card (SPI Interface)
#define SD_CS_PIN   15
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 12
#define SD_SCK_PIN  14

// MIDI Output (Hardware UART)
#define MIDI_TX_PIN 17
#define MIDI_BAUD   31250

// Rotary Encoders (ALPS EC11)
#define ENC_L_A_PIN  35
#define ENC_L_B_PIN  36
#define ENC_L_SW_PIN 37

#define ENC_R_A_PIN  38
#define ENC_R_B_PIN  39
#define ENC_R_SW_PIN 40

// Buttons
#define BTN_KIT_PIN   1
#define BTN_EDIT_PIN  2
#define BTN_MENU_PIN  42
#define BTN_CLICK_PIN 41
#define BTN_FX_PIN    40
#define BTN_SHIFT_PIN 39

// LED Outputs
#define LED_PADS_PIN      48  // WS2812B x4 (one per pad)
#define LED_ENC_DATA_PIN  47  // SK9822 Data (2 rings x 8 LEDs each)
#define LED_ENC_CLK_PIN   21  // SK9822 Clock

#define NUM_LEDS_PADS      4
#define NUM_LEDS_ENCODERS  16  // 2 rings x 8 LEDs each
#define LEDS_PER_ENCODER   8   // LEDs per encoder ring

// UART Communication to MCU #2
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define UART_BAUD   921600

// ============================================================
// HARDWARE PIN DEFINITIONS - MCU #2 (Display)
// ============================================================

#ifdef MCU_DISPLAY

// TFT Display (GC9A01)
#define TFT_MOSI       11
#define TFT_SCLK       12
#define TFT_CS         10
#define TFT_DC         8
#define TFT_RST        14
#define TFT_BACKLIGHT  2

// Touch Controller (CST816S I2C)
#define TOUCH_SDA 6
#define TOUCH_SCL 7
#define TOUCH_INT 5
#define TOUCH_RST 13

// LED Ring (WS2812B x12)
#define LED_RING_PIN     15
#define NUM_LEDS_RING    12

// UART from MCU #1
#define UART_RX_DISPLAY  33
#define UART_TX_DISPLAY  21

#endif // MCU_DISPLAY

// ============================================================
// TRIGGER DETECTION ALGORITHM PARAMETERS
// ============================================================

// Threshold Detection
#define TRIGGER_THRESHOLD_MIN 50      // Minimum ADC units above baseline to detect hit
#define TRIGGER_THRESHOLD_MAX 4000    // Maximum expected ADC value (safety clamp)

// Peak Detection Window
#define TRIGGER_SCAN_TIME_US 2000     // Time window to find peak after threshold (microseconds)

// Retrigger Suppression
#define TRIGGER_MASK_TIME_US 10000           // Mask time after hit detection (10ms)
#define TRIGGER_RETRIGGER_THRESHOLD 30       // Signal must drop below this to re-arm

// Crosstalk Rejection
#define TRIGGER_CROSSTALK_WINDOW_US 1000    // Time window to check for crosstalk (1ms)
#define TRIGGER_CROSSTALK_RATIO 0.6f        // Velocity ratio threshold for crosstalk

// Baseline Tracking (for DC offset compensation)
#define BASELINE_UPDATE_WEIGHT 1024     // Exponential moving average weight (1/1024)
#define BASELINE_INITIAL_VALUE 0        // Initial baseline value

// ADC Configuration
#define ADC_RESOLUTION 12               // 12-bit ADC (0-4095)
#define ADC_MAX_VALUE 4095
#define ADC_ATTENUATION ADC_11db        // 0-2.45V usable range

// Scan Rate
#define SCAN_PERIOD_US 500              // 500µs = 2kHz scan rate
#define SCAN_RATE_HZ 2000

// ============================================================
// VELOCITY CURVE CONFIGURATION
// ============================================================

// Velocity curve exponent (0.5 = square root, natural drum feel)
// Lower = more compression (easier to reach high velocities)
// Higher = more expansion (harder to reach high velocities)
#define VELOCITY_CURVE_EXPONENT 0.5f

// Per-pad calibration: Min/Max peak values for velocity mapping
// These should be calibrated for your specific piezo sensors
// Min = lightest tap you want to register
// Max = hardest hit you expect
const uint16_t VELOCITY_MIN_PEAK[4] = {
    100,  // Pad 0 (Kick)
    100,  // Pad 1 (Snare)
    80,   // Pad 2 (HiHat) - typically more sensitive
    100   // Pad 3 (Tom)
};

const uint16_t VELOCITY_MAX_PEAK[4] = {
    3500,  // Pad 0 (Kick)
    3500,  // Pad 1 (Snare)
    3000,  // Pad 2 (HiHat)
    3500   // Pad 3 (Tom)
};

// ============================================================
// MIDI CONFIGURATION
// ============================================================

// Default MIDI channel (9 = channel 10 in 1-based, standard drum channel)
#define DEFAULT_MIDI_CHANNEL 9

// MIDI note assignments per pad (General MIDI Drum Map)
const uint8_t PAD_MIDI_NOTES[4] = {
    36,  // Pad 0: Kick (C1 / Bass Drum 1)
    38,  // Pad 1: Snare (D1 / Acoustic Snare)
    42,  // Pad 2: HiHat (F#1 / Closed Hi-Hat)
    48   // Pad 3: Tom (C2 / Hi-Mid Tom)
};

// MIDI velocity range (1-127, 0 reserved for note-off)
#define MIDI_VELOCITY_MIN 1
#define MIDI_VELOCITY_MAX 127

// Auto Note-Off timing (milliseconds)
#define MIDI_NOTE_OFF_DELAY_MS 100

// ============================================================
// LED ANIMATION CONFIGURATION
// ============================================================

// Pad LED Colors (WS2812B)
const CRGB PAD_LED_COLORS[4] = {
    CRGB::Red,      // Pad 0: Kick
    CRGB::Blue,     // Pad 1: Snare
    CRGB::Yellow,   // Pad 2: HiHat
    CRGB::Green     // Pad 3: Tom
};

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

#define FIRMWARE_VERSION "1.0.0"
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
