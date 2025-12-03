# Quick Start - Wiring Diagram

## Stage 1: Minimal System (Pad Reading Only)

### What You Need

- 1× ESP32-S3 DevKitC-1
- 1× 5V 3A power supply
- 4× Piezo sensors (from your custom pads)
- Components for protection circuits (per pad):
  - 1× 1MΩ resistor
  - 1× 1kΩ resistor
  - 2× 1N4148 diodes
  - 1× 1µF capacitor
- 2× 1000µF capacitors (for power filtering)
- Breadboard and wires

---

## Power Distribution (Simplified)

```
┌─────────────────────────────────────────────────────────────┐
│                   5V 3A Power Supply                        │
│                  (Wall Adapter / Bench)                     │
└──────────────────────┬──────────────────────────────────────┘
                       │
           ┌───────────┴──────────────┐
           │                          │
    [1000µF Capacitor]         [Protection Diode]
           │                     (Optional: SS34)
           │                          │
    ┌──────┴──────────────────────────┴─────┐
    │         5V Power Bus (Breadboard)      │
    │            (Red Rail)                  │
    └──────┬──────────────────┬──────────────┘
           │                  │
           │                  │
     ┌─────▼──────┐    ┌──────▼───────────────┐
     │ ESP32-S3   │    │  LED Power           │
     │ VIN Pin    │    │  (Future stages)     │
     └─────┬──────┘    └──────────────────────┘
           │
     ┌─────▼──────┐
     │ ESP32-S3   │
     │ 3.3V Pin   │───────┐
     └────────────┘       │
                          │
                ┌─────────▼────────────┐
                │  Piezo Protection    │
                │  Circuits (×4)       │
                │  VCC Input           │
                └──────────────────────┘
```

---

## Step-by-Step Connections

### Step 1: Power Supply to ESP32

**Breadboard Setup**:

1. Connect **5V supply (+)** to breadboard **red rail (+)**
2. Connect **5V supply (-)** to breadboard **blue rail (-)**
3. Add **1000µF capacitor** across rails (- leg to GND)
4. Connect breadboard **red rail** to ESP32 **5V** or **VIN** pin
5. Connect breadboard **blue rail** to ESP32 **GND** pin (use 2 GND pins if possible)

**Visual**:
```
Power Supply                Breadboard                  ESP32-S3
┌──────────┐               ┌──────────┐              ┌──────────┐
│ +5V  (●) │───────────────│ + Red    │──────────────│ VIN      │
│          │               │          │              │          │
│ GND  (●) │───────────────│ - Blue   │──┬───────────│ GND      │
└──────────┘               │          │  │           │          │
                           │ [1000µF] │  └───────────│ GND      │
                           │    │ │   │              └──────────┘
                           └────┼─┼───┘
                                └─┘
                              - to +
```

### Step 2: 3.3V for Protection Circuits

**From ESP32 to Protection Circuits**:

1. Connect ESP32 **3.3V pin** to a separate breadboard rail (or row)
2. Add **100nF capacitor** from this rail to GND
3. This 3.3V rail will power all 4 protection circuits

**Visual**:
```
ESP32-S3               Protection Circuits Power
┌──────────┐          ┌────────────────────────┐
│ 3.3V     │──────────│ 3.3V Rail (Orange)     │
│          │          │                        │
│ GND      │──────────│ GND                    │
└──────────┘          │ [100nF]                │
                      │   │                    │
                      └───┼────────────────────┘
                          └─── To 4× Protection Circuits
```

### Step 3: Piezo Protection Circuit (×4)

**For EACH of the 4 pads, build this circuit**:

```
Piezo Pad #1                              ESP32-S3
┌─────────┐                               ┌──────────┐
│ Red (+) │──┬── R1 (1MΩ) ──┬─ D1 ─┬── 3.3V (from ESP32)
│         │  │               │      │
│ Blk (-) │──┴── GND         ├─ R2 (1kΩ) ──┬── ADC Pin (GPIO 4)
└─────────┘                  │              │
                             └─ D2 ─┴─── GND

                             [C1 1µF] from ADC pin to GND

Where:
- D1 = 1N4148, cathode (stripe) to 3.3V
- D2 = 1N4148, cathode to ADC pin
- R1 = 1MΩ (Brown-Black-Green)
- R2 = 1kΩ (Brown-Black-Red)
- C1 = 1µF ceramic
```

**Connection Summary**:
```
Pad 0: ADC pin GPIO 4
Pad 1: ADC pin GPIO 5
Pad 2: ADC pin GPIO 6
Pad 3: ADC pin GPIO 7
```

---

## Complete Wiring Checklist

### Power Connections
- [ ] 5V supply (+) → Breadboard red rail
- [ ] 5V supply (-) → Breadboard blue rail
- [ ] 1000µF capacitor across power rails (- to GND, + to 5V)
- [ ] Red rail → ESP32 VIN pin
- [ ] Blue rail → ESP32 GND (2 pins if possible)
- [ ] ESP32 3.3V → 3.3V distribution rail
- [ ] 100nF capacitor from 3.3V rail to GND

### Protection Circuits (×4)
- [ ] Pad 0: Circuit complete, connected to GPIO 4
- [ ] Pad 1: Circuit complete, connected to GPIO 5
- [ ] Pad 2: Circuit complete, connected to GPIO 6
- [ ] Pad 3: Circuit complete, connected to GPIO 7

### Diode Orientation Check (CRITICAL!)
- [ ] All D1 diodes: Stripe (cathode) pointing to 3.3V
- [ ] All D2 diodes: Stripe (cathode) pointing to ADC pin

### Polarity Check
- [ ] Power supply polarity verified with multimeter
- [ ] All capacitor (-)  legs to GND
- [ ] ESP32 VIN receiving 5V (measure with multimeter)
- [ ] ESP32 3.3V pin outputting 3.3V

---

## Testing Before First Power-On

### 1. Visual Inspection
- [ ] No shorts between 5V and GND (check with multimeter, power OFF)
- [ ] All connections secure (no loose wires)
- [ ] Diodes oriented correctly (stripe indicates cathode)
- [ ] Capacitors polarized correctly

### 2. Power Supply Test
- [ ] Measure supply voltage: 5.0V ± 0.25V
- [ ] Verify polarity (positive = center pin on barrel jack, typically)

### 3. Protection Circuit Test (MANDATORY)
**For EACH pad, with multimeter**:
- [ ] Idle voltage at ADC point: 0V
- [ ] Light tap: voltage rises (0.5V - 1.5V)
- [ ] Hard hit: voltage NEVER exceeds 3.3V

**DO NOT connect to ESP32 until this test passes!**

---

## First Power-On Procedure

1. **Power off ESP32** (disconnect USB if connected)
2. **Connect 5V supply** to breadboard
3. **Measure voltages**:
   - 5V rail: should be ~5.0V
   - ESP32 3.3V pin: should be ~3.3V
4. **Check current draw**: should be <200mA (ESP32 only)
5. **Connect USB** to computer
6. **Upload firmware** (see README.md)
7. **Open serial monitor** at 115200 baud
8. **Verify boot** messages appear

---

## Current Consumption Reference

| Component | Current (Idle) | Current (Active) |
|-----------|----------------|------------------|
| ESP32-S3 only | ~150mA | ~300mA |
| + 4 piezos (circuits) | <1mA | <5mA |
| **Total Stage 1** | **~150mA** | **~300mA** |

Your 5V 3A supply has plenty of headroom for this stage.

---

## Troubleshooting Quick Reference

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| ESP32 won't boot | No 5V power | Check VIN connection |
| Serial monitor blank | Wrong baud rate | Set to 115200 |
| ADC always 0 | Protection circuit broken | Check R1, piezo wiring |
| ADC always 4095 | Short circuit or no protection | **DISCONNECT IMMEDIATELY** |
| Random resets | Voltage drop | Add more capacitance (2200µF) |
| No hit detection | Threshold too high | Lower in config (default: 50) |

---

## Stage 1 Pin Summary

| Function | ESP32 Pin | Connection |
|----------|-----------|------------|
| **Power** |
| 5V Input | VIN | From 5V supply via breadboard |
| Ground | GND | To supply GND (use 2 pins) |
| 3.3V Output | 3.3V | To protection circuits VCC |
| **Piezo Triggers** |
| Pad 0 (Kick) | GPIO 4 | From protection circuit ADC out |
| Pad 1 (Snare) | GPIO 5 | From protection circuit ADC out |
| Pad 2 (HiHat) | GPIO 6 | From protection circuit ADC out |
| Pad 3 (Tom) | GPIO 7 | From protection circuit ADC out |

---

## Next Stages (Future)

### Stage 2: Add LEDs
Additional connections:
- 4× WS2812B → GPIO 48 (data)
- 16× SK9822 → GPIO 47 (data), GPIO 21 (clock) (2 rings × 8 LEDs)
- LED power: 5V via 1000µF capacitor

### Stage 3: Add MIDI
Additional connections:
- MIDI OUT circuit → GPIO 17
- 2× 220Ω resistors, DIN connector

---

## Photos Reference Points

When assembling, take photos at these stages:
1. **Power distribution** before connecting ESP32
2. **One complete protection circuit** on breadboard
3. **ESP32 connected** with power on
4. **All 4 protection circuits** complete
5. **Full system** before enclosure

This helps troubleshooting if issues arise later.

---

## Safety Reminders

⚠️ **NEVER**:
- Connect piezo directly to ESP32 (destroys ADC)
- Exceed 3.3V on any ESP32 pin
- Reverse power supply polarity
- Hot-plug piezos (power off first)

✅ **ALWAYS**:
- Test protection circuits with multimeter FIRST
- Use capacitors on power rails
- Check polarity before powering on
- Keep wires organized and labeled

---

**Quick Start Version**: 1.0
**For**: Stage 1 - Pad Reading
**Hardware**: ESP32-S3 DevKitC-1
**Power**: 5V 3A supply

**Need more details?** See [docs/power_distribution.md](power_distribution.md) and [docs/hardware_assembly.md](hardware_assembly.md)
