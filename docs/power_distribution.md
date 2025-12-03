# E-Drum Controller - Power Distribution Guide

## Overview

This guide covers the power distribution strategy for the E-Drum controller using a single **5V 3A power supply** to power:
- 2× ESP32-S3 modules (MCU#1 Main Brain + MCU#2 Display)
- 4× WS2812B LEDs (pads)
- 16× SK9822 LEDs (encoder rings, 2 rings × 8 LEDs)
- 12× WS2812B LEDs (display ring - future)
- All peripheral circuits

---

## Power Budget Analysis

### Current Consumption Breakdown

| Component | Voltage | Current (Typ) | Current (Max) | Notes |
|-----------|---------|---------------|---------------|-------|
| **ESP32-S3 #1 (Main Brain)** | 5V | 150mA | 500mA | Peak during WiFi/BT (not used yet) |
| **ESP32-S3 #2 (Display)** | 5V | 150mA | 500mA | Future phase |
| **WS2812B Pads (4 LEDs)** | 5V | 60mA | 240mA | 60mA per LED at full white |
| **SK9822 Rings (16 LEDs)** | 5V | 60mA | 320mA | 20mA per LED average (2 rings × 8 LEDs) |
| **WS2812B Display Ring (12 LEDs)** | 5V | 40mA | 720mA | Future phase |
| **Protection Circuits** | 3.3V | <1mA | <5mA | Negligible |
| **DAC PCM5102 (Audio)** | 5V | 10mA | 50mA | Future phase |
| **SD Card** | 3.3V | 20mA | 100mA | During write operations |
| **Encoders/Buttons** | 3.3V | <1mA | <5mA | Negligible |
| | | | | |
| **TOTAL (Stage 1)** | 5V | **~380mA** | **~1.1A** | Current implementation |
| **TOTAL (All Stages)** | 5V | **~530mA** | **~2.5A** | Full system with all LEDs |

### Safety Margin

- **Available**: 3A
- **Typical consumption**: ~530mA (18%)
- **Peak consumption**: ~2.5A (83%)
- **Safety margin**: ~500mA (17%)

**Conclusion**: A 5V 3A supply is **adequate** for the full system, assuming LED brightness is limited to reasonable levels (not all LEDs at full white simultaneously).

---

## Power Distribution Architecture

### Recommended Topology: Star Distribution

```
                    [5V 3A Power Supply]
                            │
                    ┌───────┴───────┐
                    │               │
              [Main Power Bus]      │
                    │               │
        ┌───────────┼───────────────┼──────────┐
        │           │               │          │
        │           │               │          │
    [ESP32-S3]  [ESP32-S3]   [LED Power]   [Future]
     MCU #1       MCU #2      Distribution  Expansion
     (5V IN)    (5V IN)         (5V)
        │           │               │
        ├──3.3V─────┤               │
        │           │               │
    [Piezo      [Sensors/      [4× WS2812B]
     Protection  Peripherals]   [16× SK9822]
     Circuits]                  [12× WS2812B]
```

---

## Implementation Guide

### Option 1: Breadboard Prototype (Testing)

#### Components Needed

| Qty | Component | Purpose | Example |
|-----|-----------|---------|---------|
| 1 | 5V 3A power supply | Main power | Wall adapter with barrel jack |
| 1 | Barrel jack connector | Power input | 5.5mm × 2.1mm |
| 2 | Electrolytic capacitor 1000µF 16V | Bulk filtering | Radial, low ESR |
| 4 | Ceramic capacitor 100nF 50V | High-frequency filtering | X7R or better |
| 1 | Schottky diode 3A | Reverse polarity protection | 1N5822 or SS34 |
| 1 | Power distribution board | Optional but recommended | Breadboard power module |

#### Breadboard Power Distribution

**Power Rails Setup**:

1. **5V Main Rail** (Red rail):
   ```
   [5V Supply +] ──┬── [D1 Schottky] ──┬── [C1 1000µF] ── [5V Rail]
                   │                    │
   [GND Supply]  ──┴───────────────────┴── [GND Rail]
   ```

2. **ESP32-S3 #1 Connection**:
   ```
   [5V Rail] ────────── ESP32 5V/VIN pin
   [GND Rail] ───────── ESP32 GND pin

   ESP32 3.3V OUT ───┬── [C2 100nF] ── GND
                     │
                     └── To piezo protection circuits VCC
   ```

3. **LED Power Connection**:
   ```
   [5V Rail] ───┬── [C3 1000µF] ──┬── WS2812B VCC (4 LEDs)
                │                 │
                ├─────────────────┼── SK9822 VCC (32 LEDs)
                │                 │
   [GND Rail] ──┴─────────────────┴── All LED GND
   ```

**CRITICAL**: Add 1000µF capacitor close to LED power input to prevent voltage dips during LED updates.

---

### Option 2: Permanent Installation (Perfboard/PCB)

#### Recommended Circuit

```
5V IN ──┬── F1 (3A Fuse) ──┬── D1 (SS34) ──┬──┬── C1 (1000µF) ──┬── 5V BUS
        │                  │               │  │                  │
        │                  │               │  └── C2 (100nF) ────┤
        │                  │               │                     │
        │                  │               ├── ESP32-S3 #1 VIN  │
        │                  │               │                     │
        │                  │               ├── ESP32-S3 #2 VIN  │
        │                  │               │   (future)          │
        │                  │               │                     │
        │                  │               ├──┬── C3 (1000µF) ──┬── LED 5V
        │                  │               │  └── C4 (100nF) ───┤
GND ────┴──────────────────┴───────────────┴────────────────────┴── GND BUS
```

#### Component Details

| Component | Value | Purpose | Part Number Example |
|-----------|-------|---------|---------------------|
| **F1** | 3A resettable fuse (PTC) | Overcurrent protection | Bourns MF-R300 |
| **D1** | Schottky diode 3A 40V | Reverse polarity protection | SS34, 1N5822 |
| **C1** | 1000µF 16V electrolytic | Bulk capacitance (ESP32 power) | Low ESR |
| **C2** | 100nF ceramic | High-frequency filtering | X7R, 50V |
| **C3** | 1000µF 16V electrolytic | Bulk capacitance (LED power) | Low ESR |
| **C4** | 100nF ceramic | High-frequency filtering | X7R, 50V |

#### Bill of Materials (BOM)

| Qty | Component | Cost (USD) |
|-----|-----------|------------|
| 1 | 3A PTC fuse | $0.50 |
| 1 | SS34 Schottky diode | $0.20 |
| 2 | 1000µF 16V capacitor | $0.80 |
| 4 | 100nF ceramic capacitor | $0.20 |
| 1 | Barrel jack connector | $0.50 |
| 1 | Perfboard or terminal block | $2.00 |
| - | Wire (18-22 AWG) | $1.00 |
| - | **TOTAL** | **~$5.20** |

---

## Wiring Guidelines

### Wire Gauge Selection

| Connection | Current | Recommended Wire Gauge | Max Length |
|------------|---------|------------------------|------------|
| **5V Main Bus** | 3A | 18 AWG (1mm²) | <50cm |
| **ESP32 5V** | 500mA | 22 AWG (0.5mm²) | <30cm |
| **LED 5V** | 1.5A | 20 AWG (0.75mm²) | <30cm |
| **GND Return** | 3A | 18 AWG (1mm²) | <50cm |
| **3.3V (ESP32 out)** | 500mA | 24 AWG (0.25mm²) | <20cm |

### Best Practices

1. **Keep wires short**: Minimize voltage drop (V_drop = I × R)
2. **Twist power pairs**: Reduce EMI
3. **Star ground**: All grounds meet at one point (power supply GND)
4. **Separate analog ground**: Optional - connect piezo circuit GND at ESP32 GND pin
5. **Avoid loops**: Don't create ground loops (causes noise)

### Color Coding (Recommended)

| Color | Use |
|-------|-----|
| **Red** | 5V power |
| **Black** | GND |
| **Orange** | 3.3V |
| **Yellow** | Signal/Data |
| **Blue** | Analog (piezo) |

---

## Specific Connections for Stage 1

### MCU #1 (Main Brain)

**Power Input**:
```
5V Bus ──────── ESP32-S3 Pin "5V" or "VIN"
GND Bus ─────── ESP32-S3 Pin "GND" (connect at least 2 GND pins)
```

**3.3V Output** (for protection circuits):
```
ESP32-S3 "3.3V" pin ──┬── To piezo protection VCC (4 circuits)
                      │
                      └── [100nF cap] ── GND
```

**Note**: ESP32-S3 3.3V regulator can supply up to ~500mA. Piezo circuits use <5mA, so this is safe.

### LED Connections

**WS2812B Pads (4 LEDs)**:
```
5V Bus ──┬── [1000µF cap] ──┬── LED VCC (daisy chain)
         │                  │
GND Bus ─┴──────────────────┴── LED GND

ESP32 GPIO 48 ───────────────── LED DATA IN (first LED)
```

**SK9822 Encoder Rings (32 LEDs)**:
```
5V Bus ──┬── [1000µF cap] ──┬── LED VCC (daisy chain)
         │                  │
GND Bus ─┴──────────────────┴── LED GND

ESP32 GPIO 47 ───────────────── LED DATA IN
ESP32 GPIO 21 ───────────────── LED CLOCK IN
```

**IMPORTANT**: Add 330Ω resistor in series with DATA line to reduce reflections:
```
ESP32 GPIO 48 ── [330Ω] ── WS2812B DATA IN
ESP32 GPIO 47 ── [330Ω] ── SK9822 DATA IN
```

---

## Protection Circuits Power

### 3.3V Rail for Piezos

Each piezo protection circuit needs 3.3V for the clamping diodes:

```
ESP32 3.3V OUT ──┬── [100nF cap] ── GND
                 │
                 ├── To Piezo Protection #1 VCC
                 ├── To Piezo Protection #2 VCC
                 ├── To Piezo Protection #3 VCC
                 └── To Piezo Protection #4 VCC
```

**Current calculation**:
- Each protection circuit: ~1µA (just diode leakage)
- Total: <5µA negligible

**Capacitor placement**: Add one 100nF capacitor close to the protection circuits.

---

## Testing Procedure

### Before Connecting Components

1. **Measure supply voltage**:
   - Expected: 5.0V ± 0.25V (4.75V - 5.25V acceptable)
   - Use multimeter on DC voltage mode

2. **Check polarity**:
   - Center pin of barrel jack is typically **positive**
   - Outer sleeve is typically **negative**
   - **VERIFY with multimeter before connecting!**

3. **Load test**:
   - Connect 10Ω 5W resistor across 5V and GND
   - Expected current: 5V / 10Ω = 500mA
   - Voltage should remain >4.8V under load

### After Connecting Power Distribution

1. **No-load test**:
   - Connect only power distribution board (no ESP32 or LEDs)
   - Measure 5V rail: should be 5.0V ± 0.1V
   - Measure current: should be <10mA

2. **ESP32 test**:
   - Connect ESP32-S3 only (no LEDs)
   - Power on
   - Measure 5V rail: should remain >4.8V
   - Measure 3.3V pin: should be 3.3V ± 0.1V
   - ESP32 should boot normally (LED blinks)

3. **LED test**:
   - Connect LED strips
   - Power on
   - Set LEDs to low brightness (~10%)
   - Measure 5V rail under load: should be >4.7V
   - Increase brightness gradually while monitoring voltage

4. **Full system test**:
   - All components connected
   - Upload test firmware
   - Monitor current consumption with multimeter in series
   - Expected: 300-500mA at idle, <1.5A with LEDs active

---

## Common Issues & Solutions

### Issue: ESP32 resets randomly

**Cause**: Voltage drop when LEDs update (capacitance insufficient)

**Solution**:
- Add larger capacitor (2200µF or 3300µF) near LED power
- Reduce LED brightness
- Limit number of LEDs updated simultaneously

### Issue: LEDs flicker or show wrong colors

**Cause**: Insufficient current or voltage drop

**Solution**:
- Check wire gauge (should be ≥20 AWG for LED power)
- Add capacitor at LED strip input
- Measure voltage at LED strip: should be >4.5V
- Reduce brightness or number of LEDs

### Issue: 3.3V rail drops below 3.2V

**Cause**: ESP32 internal regulator overloaded

**Solution**:
- Verify piezo protection circuits are using <5mA
- Check for short circuits
- Ensure 100nF capacitor is close to 3.3V pin

### Issue: Power supply gets hot

**Cause**: Overcurrent or short circuit

**Solution**:
- Measure total current consumption
- Check for short circuits with multimeter (power OFF)
- Ensure supply is rated for continuous 3A (not peak)
- Add heatsink to supply if needed

### Issue: Voltage drops below 4.7V

**Cause**: Wire resistance too high or supply inadequate

**Solution**:
- Use thicker wire (18 AWG for main bus)
- Shorten wire runs
- Verify power supply can actually deliver 3A continuously
- Measure voltage at power supply terminals vs. ESP32

---

## Advanced: PCB Power Distribution Layout

If designing a custom PCB for power distribution:

### Design Rules

1. **Trace width for 3A @ 1oz copper**:
   - Minimum: 3mm (120 mil)
   - Recommended: 5mm (200 mil) or use power plane

2. **Copper planes**:
   - Top layer: 5V power plane
   - Bottom layer: GND plane
   - Connect with multiple vias (at least 10× 0.6mm vias)

3. **Capacitor placement**:
   - Bulk (1000µF): Within 2cm of load
   - Ceramic (100nF): Within 5mm of IC power pins

4. **Thermal management**:
   - Schottky diode: 20mm² copper pad for heatsinking
   - Fuse: Exposed copper for cooling

### Example Layout (Top View)

```
  [Barrel Jack] ─── [Fuse] ─── [Diode] ─── [5V Plane] ───┬─── [ESP32 #1]
                                   │                      │
                                  [C1]                    ├─── [ESP32 #2]
                                   │                      │
                               [GND Plane] ───────────────┴─── [LED Power]
```

---

## Shopping List for Power System

### Minimal Setup (Breadboard Testing)

- [ ] 5V 3A power supply with barrel jack
- [ ] Breadboard power module (optional but helpful)
- [ ] 2× 1000µF 16V electrolytic capacitors
- [ ] 4× 100nF ceramic capacitors
- [ ] 1× SS34 Schottky diode
- [ ] 22 AWG hookup wire (red, black)
- [ ] Multimeter (if you don't have one)

**Total cost**: ~$10-15 USD

### Complete Setup (Permanent)

Add to minimal:
- [ ] 1× 3A PTC resettable fuse
- [ ] Perfboard or custom PCB
- [ ] 2× Additional 1000µF capacitors (for LEDs)
- [ ] Terminal blocks (screw terminals)
- [ ] 18 AWG wire for high-current connections
- [ ] Heat shrink tubing
- [ ] Enclosure with mounting holes

**Total cost**: ~$20-30 USD

---

## Power Consumption Optimization

### LED Brightness Limiting

In `edrum_config.h`, limit LED brightness to reduce current:

```cpp
// Pad LED idle brightness (0-255)
#define PAD_LED_IDLE_BRIGHTNESS 30  // ~12% = 7mA per LED vs 60mA at full

// Encoder LED max brightness
#define ENCODER_LED_BRIGHTNESS_MAX 0.4f  // 40% = 24mA per LED vs 60mA
```

**Current savings**:
- Pad LEDs: 4 × (60 - 7) = 212mA saved
- Encoder LEDs: 16 × (60 - 24) = 576mA saved
- **Total**: ~790mA saved!

### FastLED Global Brightness

Use FastLED global brightness limiter:

```cpp
FastLED.setBrightness(64);  // 25% of maximum (0-255 scale)
```

This ensures even accidental full-brightness commands won't overdraw current.

---

## Expansion Considerations

### Adding MCU #2 (Display)

When implementing Stage 4:
- Additional 150mA typical (500mA peak)
- Powered from same 5V bus
- 12× WS2812B ring adds 40mA typical (720mA peak)

**Action**: Ensure LED brightness is limited to keep total <2.8A

### Adding Audio (I2S DAC)

PCM5102 DAC module:
- ~10mA idle, 50mA peak
- Powered from 5V bus
- Negligible impact on power budget

### Future: Battery Operation

If you want battery power later:
- Use 2S LiPo (7.4V nominal) with 5V buck converter
- Capacity needed: 2000mAh for ~3 hours @ 500mA average
- Converter: XL4015 or similar (3A capable)

---

## Safety Checklist

Before powering on:

- [ ] Power supply rated for 5V 3A continuous (not peak)
- [ ] Polarity verified with multimeter
- [ ] Fuse installed (if using)
- [ ] Schottky diode oriented correctly (stripe = cathode to 5V)
- [ ] All capacitors oriented correctly (- leg to GND)
- [ ] No short circuits (check with multimeter continuity mode)
- [ ] ESP32 5V and GND connected correctly
- [ ] LED power connections correct (VCC to 5V, GND to GND)
- [ ] Wire gauge adequate (18-22 AWG)
- [ ] Multimeter ready to measure voltage under load

---

## Recommended Power Supplies

### Desktop/Bench Testing

| Model | Voltage | Current | Connector | Price |
|-------|---------|---------|-----------|-------|
| Mean Well RS-15-5 | 5V | 3A | Screw terminal | $10 |
| Generic 5V 3A adapter | 5V | 3A | Barrel jack 5.5×2.1mm | $8 |
| Laptop USB-C PD trigger | 5V | 3A | USB-C (with trigger board) | $12 |

### Portable/Battery

| Model | Voltage | Capacity | Notes | Price |
|-------|---------|----------|-------|-------|
| USB Power Bank 10000mAh | 5V | 2A | Need USB cable, limited current | $15 |
| 2S LiPo + 5V Buck Converter | 5V | 3A+ | Requires charging circuit | $20 |

---

**Document Version**: 1.0
**Last Updated**: 2025-12-02
**Validated With**: ESP32-S3 DevKitC-1, 5V 3A wall adapter

**REMINDER**: Always use a fuse or PTC for protection. Test voltage and polarity before connecting!
