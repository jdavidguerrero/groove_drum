# E-Drum Controller - Firmware

Firmware for a professional-grade electronic drum controller with dual ESP32-S3 architecture.

## Project Status

**Current Stage**: STAGE 1 - Pad Reading (Ready to compile and test)

### Implemented Features

✅ **Complete Infrastructure**:
- PlatformIO build configuration
- Dual-MCU project structure
- Shared configuration and protocol libraries

✅ **Stage 1 - Trigger Detection**:
- Real-time ADC scanning at 2kHz (500µs period)
- Velocity-sensitive peak detection (MIDI 1-127)
- Logarithmic velocity curve for natural feel
- Crosstalk rejection between pads
- Adaptive baseline tracking
- FreeRTOS dual-core architecture (Core 0: scanning, Core 1: processing)

✅ **Safety Features**:
- ADC safety limit monitoring
- Hardware protection circuit documentation
- Comprehensive testing procedures

### Planned Features

⏳ **Stage 2** - LED Animations (WS2812B + SK9822)
⏳ **Stage 3** - MIDI Output
⏳ **Stage 4** - GUI on MCU#2 (LVGL)
⏳ **Stage 5** - I2S Audio Generation (PCM5102)

---

## Hardware Requirements

### MCU #1 (Main Brain)
- **Board**: ESP32-S3 DevKitC-1 or compatible
- **Features**:
  - 4× Piezo triggers (ADC1)
  - 2× ALPS encoders
  - 6× Buttons
  - SD card (SPI)
  - 4× WS2812B LEDs (pads)
  - 16× SK9822 LEDs (encoder rings, 2 rings × 8 LEDs)
  - MIDI OUT (UART)
  - UART to MCU#2

### Power Supply
- **Required**: 5V 3A power supply (wall adapter)
- **Connector**: Barrel jack (5.5mm × 2.1mm) or screw terminals
- **Consumption**:
  - Stage 1 (current): ~380mA typical, ~1.4A peak
  - Full system: ~550mA typical, ~2.8A peak
- **See**: [docs/power_distribution.md](docs/power_distribution.md) for complete wiring guide

### ⚠️ CRITICAL: Piezo Protection Circuits

**DO NOT connect piezo sensors directly to ESP32!**

Each piezo MUST have a protection circuit:
- 1× 1MΩ resistor
- 1× 1kΩ resistor
- 2× 1N4148 diodes
- 1× 1µF capacitor

**See [docs/hardware_assembly.md](docs/hardware_assembly.md) for detailed instructions!**

Failure to use protection circuits will **permanently damage** the ESP32-S3.

### Quick Start - Hardware Setup

1. **Power distribution** ([docs/power_distribution.md](docs/power_distribution.md)):
   ```
   5V 3A Supply ──┬── ESP32-S3 #1 (VIN pin)
                  ├── LED Power (via 1000µF cap)
                  └── Future: ESP32-S3 #2

   ESP32 3.3V ────── Piezo protection circuits VCC
   ```

2. **Piezo protection circuits** (×4, one per pad):
   ```
   Piezo ── 1MΩ ── ┬── Diodes ── To ESP32 ADC
                   └── 1kΩ ───── GND
   ```

3. **LED connections**:
   ```
   5V ──[1000µF]── WS2812B VCC (GPIO 48 data)
   5V ──[1000µF]── SK9822 VCC (GPIO 47 data, 21 clock)
   ```

---

## Build Instructions

### Prerequisites

- PlatformIO Core or PlatformIO IDE (VS Code extension)
- USB cable for ESP32-S3
- Components for piezo protection circuits

### Compile and Upload

1. **Clone or navigate to project**:
   ```bash
   cd /path/to/groove_drum
   ```

2. **Build for Main Brain (MCU#1)**:
   ```bash
   platformio run -e main_brain
   ```

3. **Upload to ESP32-S3**:
   ```bash
   platformio run -e main_brain -t upload
   ```

4. **Monitor serial output**:
   ```bash
   platformio device monitor -b 115200
   ```

### Build Options

- **Main Brain only**: `platformio run -e main_brain`
- **Display only** (future): `platformio run -e display`
- **Clean build**: `platformio run -t clean`

---

## Testing Guide

### Stage 1: Pad Reading Tests

After uploading firmware:

1. **Initial Boot**:
   - Verify system initialization messages
   - Check that all 4 ADC channels show safe idle values (0-100)
   - Confirm FreeRTOS tasks started on Core 0 and Core 1

2. **ADC Test** (Serial command: `a`):
   - Verifies ADC readings at idle
   - Ensures no values exceed safety limit
   - Checks for non-zero baseline

3. **Hit Detection Test**:
   - Strike each pad with varying force
   - Observe serial output: `>> HIT: Pad=X, Velocity=Y`
   - Verify velocity increases with strike force

4. **Velocity Curve Test**:
   - Light tap: velocity 10-30
   - Medium hit: velocity 50-80
   - Hard hit: velocity 100-127

5. **Crosstalk Test**:
   - Strike Pad 0, verify Pad 1 doesn't trigger
   - Repeat for all adjacent pad combinations

6. **Rapid Retriggering Test**:
   - Perform fast drum roll (~10 hits/sec)
   - Verify all hits detected (no missed triggers)

### Serial Commands

While firmware is running, press these keys in serial monitor:

| Key | Command | Description |
|-----|---------|-------------|
| `h` | Help | Show all commands |
| `s` | System Info | Display hardware config and firmware version |
| `t` | Trigger State | Show current state of all pads |
| `a` | ADC Test | Read and display raw ADC values |
| `r` | Reset Triggers | Reset all pad state machines to IDLE |
| `m` | Scan Stats | Show ADC scan timing statistics |
| `c` | Clear Stats | Reset scan timing statistics |

---

## Project Structure

```
groove_drum/
├── platformio.ini           # Build configuration
├── README.md                # This file
├── docs/
│   └── hardware_assembly.md # Piezo protection circuit guide
├── shared/                  # Code shared between MCU#1 and MCU#2
│   ├── config/
│   │   └── edrum_config.h   # Pin definitions, tuning parameters
│   └── protocol/
│       ├── protocol.h       # UART protocol structures
│       └── protocol.cpp     # CRC-8, frame encoding
└── src/
    └── main_brain/          # MCU#1 firmware
        ├── main.cpp         # Entry point, FreeRTOS setup
        ├── core/
        │   ├── system_config.h/.cpp   # Hardware initialization
        └── input/
            ├── trigger_scanner.h/.cpp  # ADC scanning loop (Core 0)
            └── trigger_detector.h/.cpp # Peak detection algorithm
```

---

## Configuration

### Tuning Trigger Sensitivity

Edit `shared/config/edrum_config.h`:

```c
// Threshold for hit detection (ADC units above baseline)
#define TRIGGER_THRESHOLD_MIN 50

// Peak detection window (microseconds)
#define TRIGGER_SCAN_TIME_US 2000

// Velocity curve: 0.5 = natural, 0.3 = easier, 0.8 = harder
#define VELOCITY_CURVE_EXPONENT 0.5f
```

### Per-Pad Calibration

Calibrate min/max peak values for each pad:

```c
const uint16_t VELOCITY_MIN_PEAK[4] = {
    100,  // Pad 0: lightest tap to register
    100,  // Pad 1
    80,   // Pad 2: HiHat (more sensitive)
    100   // Pad 3
};

const uint16_t VELOCITY_MAX_PEAK[4] = {
    3500,  // Pad 0: hardest expected hit
    3500,  // Pad 1
    3000,  // Pad 2
    3500   // Pad 3
};
```

### MIDI Note Mapping

Change which MIDI notes each pad triggers:

```c
const uint8_t PAD_MIDI_NOTES[4] = {
    36,  // Pad 0: Kick (C1)
    38,  // Pad 1: Snare (D1)
    42,  // Pad 2: HiHat (F#1)
    48   // Pad 3: Tom (C2)
};
```

---

## Troubleshooting

### Issue: No serial output

**Causes**:
- USB cable not data-capable (charge-only)
- Wrong serial port selected
- Wrong baud rate (should be 115200)

**Solution**:
- Try different USB cable
- Check PlatformIO device monitor settings
- Press ESP32 reset button

### Issue: "ADC exceeds safety limit"

**Causes**:
- **CRITICAL**: Protection circuit missing or failed
- Piezo generating too much voltage

**Solution**:
- **IMMEDIATELY disconnect piezo**
- Verify protection circuit with multimeter
- Check all diode orientations
- Do NOT reconnect until validated

### Issue: Inconsistent trigger detection

**Causes**:
- Loose piezo mounting
- EMI from nearby wiring
- Threshold too high/low

**Solution**:
- Secure piezo mechanically
- Twist piezo wires together
- Adjust `TRIGGER_THRESHOLD_MIN`

### Issue: False triggers (ghost notes)

**Causes**:
- Crosstalk between pads
- Mechanical vibration
- Electrical noise

**Solution**:
- Increase `TRIGGER_CROSSTALK_WINDOW_US`
- Physically isolate pads
- Add more damping material

---

## Development Roadmap

### ✅ Phase 1: Core Infrastructure (COMPLETE)
- Project structure
- Configuration system
- Protocol library

### ✅ Phase 2: Stage 1 - Pad Reading (COMPLETE)
- ADC scanning at 2kHz
- Peak detection algorithm
- Velocity mapping
- Crosstalk rejection

### ⏳ Phase 3: Stage 2 - LED Animations
- WS2812B pad LEDs (flash on hit)
- SK9822 encoder rings (breathing animation)
- FreeRTOS LED task

### ⏳ Phase 4: Stage 3 - MIDI Output
- Hardware MIDI OUT circuit
- MIDI Note On/Off generation
- Auto note-off timing

### ⏳ Phase 5: Stage 4 - GUI (MCU#2)
- LVGL configuration
- 4 UI screens (Performance, Edit, Mixer, Settings)
- Touch input (CST816S)
- UART communication

### ⏳ Phase 6: Stage 5 - Audio Generation
- I2S DAC setup (PCM5102)
- WAV sample playback
- Multi-voice mixing

---

## Contributing

This is a personal project, but suggestions and bug reports are welcome!

**Areas for improvement**:
- Velocity curve algorithms
- Crosstalk rejection strategies
- Dual-zone piezo support
- Alternative trigger algorithms

---

## License

Copyright © 2025 - E-Drum Controller Project

This firmware is provided for educational and personal use.

---

## Safety & Disclaimers

- **Electrical Safety**: Always use proper protection circuits for piezo sensors
- **ESD Protection**: Handle ESP32 boards with proper anti-static precautions
- **Power Supply**: Use quality USB power supplies (2A minimum)
- **Firmware Updates**: Always backup configurations before updating

**The developers are not responsible for hardware damage resulting from improper circuit assembly or use.**

---

## Contact & Support

For issues related to:
- **Firmware bugs**: Check serial output, provide full logs
- **Hardware problems**: Review [docs/hardware_assembly.md](docs/hardware_assembly.md)
- **Configuration questions**: See "Configuration" section above

---

**Firmware Version**: 1.0.0
**Build Date**: 2025-12-02
**Compatible Hardware**: ESP32-S3 DevKitC-1
**Status**: Stage 1 Complete - Ready for Testing
