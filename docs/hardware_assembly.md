# E-Drum Controller - Hardware Assembly Guide

## ⚠️ CRITICAL WARNING

**DO NOT connect piezo sensors directly to ESP32-S3 ADC pins!**

Piezo sensors generate voltage spikes of **20-40V** when struck. The ESP32-S3 ADC has an **absolute maximum rating of 3.6V**. Direct connection **WILL INSTANTLY DESTROY** the ADC and potentially damage the entire microcontroller.

**Protection circuits are MANDATORY before connecting any piezo to the ESP32!**

---

## Protection Circuit Design

### Circuit Schematic (Per Pad)

```
Piezo (+) ──┬─── R1 (1MΩ) ───┬─── D1 (1N4148) ─┬─── VCC (3.3V)
            │                 │                 │
            │                 ├─── R2 (1kΩ) ────┼─── To ESP32 ADC Pin
            │                 │                 │
Piezo (-) ──┴─── GND          │                 │
                               └─── D2 (1N4148) ─┴─── GND

Optional but recommended: C1 (100nF-1µF) between ADC pin and GND
```

### Component Functions

| Component | Value | Purpose | Why It's Critical |
|-----------|-------|---------|-------------------|
| **R1** | 1MΩ, 1/4W | Series current limiter | Limits maximum current from piezo spike to safe levels (~3µA at 3.3V) |
| **R2** | 1kΩ, 1/4W | Pull-down + filtering | Ensures ADC sees 0V when no signal; forms RC low-pass filter with C1 |
| **D1** | 1N4148 | Positive voltage clamp | Clamps voltage to VCC + 0.6V (max 3.9V) - safe for ESP32 |
| **D2** | 1N4148 | Negative voltage clamp | Prevents reverse polarity damage (piezo can generate negative voltage) |
| **C1** | 100nF-1µF ceramic | Anti-aliasing filter | Reduces high-frequency noise; cutoff frequency ~160Hz @ 1µF |

### Alternative: Zener Diode Circuit (More Robust)

If you prefer a simpler design:

```
Piezo (+) ──┬─── R1 (1MΩ) ───┬─── R2 (1kΩ) ───┬─── To ESP32 ADC Pin
            │                 │                │
Piezo (-) ──┴─── GND          ├─── ZD1 (3.3V) ─┴─── GND
                               │
                               └─── C1 (1µF) ───── GND
```

- **ZD1**: 3.3V Zener diode (e.g., BZX55C3V3)
- **Advantage**: Bidirectional clamping in one component
- **Disadvantage**: Slightly loads the signal more than dual diode approach

---

## Bill of Materials

### For 4 Pads (Full System)

| Qty | Component | Part Number Example | Where to Buy | Unit Price | Total |
|-----|-----------|---------------------|--------------|------------|-------|
| 4 | 1MΩ resistor 1/4W 5% | CFR-25JT-52-1M0 | DigiKey, Mouser, Amazon | $0.05 | $0.20 |
| 4 | 1kΩ resistor 1/4W 5% | CFR-25JT-52-1K0 | DigiKey, Mouser, Amazon | $0.05 | $0.20 |
| 8 | 1N4148 diode | 1N4148 | DigiKey, Mouser, Amazon | $0.05 | $0.40 |
| 4 | Ceramic capacitor 1µF 50V | Generic | DigiKey, Mouser, Amazon | $0.10 | $0.40 |
| - | **TOTAL** | - | - | - | **$1.20** |

### Optional Components

| Qty | Component | Purpose | Price |
|-----|-----------|---------|-------|
| 1 | Breadboard 830 points | Prototyping/testing | $5 |
| 1 | Perfboard/stripboard | Permanent assembly | $3 |
| 20 | Jumper wires M-M | Connections | $3 |
| 1 | Multimeter | CRITICAL for testing | $15-30 |
| 4 | 2-pin screw terminals | Piezo connections | $2 |

---

## Assembly Instructions

### Step 1: Component Preparation

1. **Gather all components** listed in the BOM
2. **Verify component values** with a multimeter:
   - R1 = 1MΩ (Brown-Black-Green or 1005)
   - R2 = 1kΩ (Brown-Black-Red or 1002)
3. **Test diodes** - check forward voltage drop (~0.6V) with multimeter diode mode

### Step 2: Build on Breadboard (TESTING ONLY)

#### Circuit 1 (Pad 0 - GPIO 4)

1. Insert **R1 (1MΩ)** across rows (e.g., E5-F10)
2. Insert **R2 (1kΩ)** from R1 end to ADC connection row (F10-F15)
3. Insert **D1 (1N4148)** with cathode (stripe) to 3.3V rail, anode to ADC row
4. Insert **D2 (1N4148)** with cathode to ADC row, anode to GND rail
5. Insert **C1 (1µF)** between ADC row and GND rail
6. Connect piezo (+) wire to R1 input end
7. Connect piezo (-) wire to GND rail
8. **DO NOT CONNECT TO ESP32 YET!**

#### Repeat for Pads 1, 2, 3

Build identical circuits for the other 3 pads.

### Step 3: Testing (MANDATORY)

#### Test Setup

1. Connect breadboard GND rail to ESP32 GND
2. Connect breadboard 3.3V rail to ESP32 3.3V
3. **DO NOT** connect ADC pins yet
4. Set multimeter to DC voltage mode

#### Test Procedure

For **EACH PAD**:

1. **Idle Test**:
   - Place multimeter probes on ADC connection point and GND
   - Piezo at rest (not touching)
   - **Expected**: 0V ± 0.05V
   - **If not**: Check R2 connection to GND

2. **Light Tap Test**:
   - Gently tap piezo
   - Observe multimeter (may need max hold function)
   - **Expected**: 0.5V - 1.5V peak
   - **If not**: Check R1 connection

3. **Hard Hit Test** (CRITICAL):
   - Strike piezo **as hard as possible**
   - Observe multimeter max value
   - **Expected**: 2.5V - 3.3V peak
   - **MUST NOT EXCEED**: 3.3V
   - **If >3.3V**: Re-check all diodes and connections. DO NOT PROCEED.

4. **Reverse Polarity Test**:
   - Swap piezo +/- wires
   - Strike piezo hard
   - **Expected**: Voltage should still be within 0-3.3V range
   - **If negative voltage**: Check D2 orientation

#### Safety Validation Checklist

- [ ] All 4 pads show 0V at idle
- [ ] All 4 pads respond to taps (voltage increases with force)
- [ ] All 4 pads **NEVER exceed 3.3V** even with hardest hit
- [ ] Voltage returns to 0V after ~100ms
- [ ] No negative voltages observed

**ONLY PROCEED TO STEP 4 IF ALL CHECKS PASS!**

### Step 4: Connect to ESP32

Once all tests pass:

1. Power off ESP32
2. Connect each ADC output point to corresponding GPIO:
   - Pad 0 → GPIO 4
   - Pad 1 → GPIO 5
   - Pad 2 → GPIO 6
   - Pad 3 → GPIO 7
3. Double-check all connections
4. Power on ESP32
5. Upload firmware and test

---

## Permanent Assembly (After Testing)

### Option 1: Perfboard

**Advantages**:
- Permanent, reliable connections
- Good for final installation

**Steps**:
1. Transfer circuit from breadboard to perfboard
2. Solder all components
3. Use screw terminals for piezo connections
4. Add strain relief for piezo wires
5. Mount in enclosure

### Option 2: Custom PCB (Advanced)

For a professional finish, design a PCB with:
- 4 identical protection circuits
- Screw terminals for piezo inputs
- 8-pin header for ESP32 connection (4× ADC + GND + 3.3V)
- Ground plane for noise reduction
- Compact layout (<5cm × 5cm)

**Tools**: KiCad (free), EasyEDA, or similar

---

## Troubleshooting

### Problem: ADC reads 0V even when hitting piezo

**Possible causes**:
- R1 (1MΩ) is open circuit → check with multimeter
- Piezo disconnected → check wiring
- Piezo damaged → test with multimeter in capacitance mode (~20-40nF typical)

**Solution**: Check continuity of all connections, replace suspect components

### Problem: ADC reads near 3.3V at idle

**Possible causes**:
- R2 not connected to GND
- D1 reversed (cathode should go to VCC)

**Solution**: Verify R2 and D1 connections

### Problem: Inconsistent readings (flickers between 0V and random values)

**Possible causes**:
- Missing or too-small C1 capacitor
- Loose connections on breadboard
- EMI pickup (wires too long)

**Solution**:
- Add 1µF capacitor if not present
- Use shorter wires
- Twist piezo wires together

### Problem: ESP32 ADC damaged (always reads 0 or 4095)

**Cause**: Voltage spike exceeded 3.6V before protection circuit was in place

**Solution**:
- **Immediate**: Stop using that ADC pin
- **Test other pins**: Use GPIO 1 or other ADC1 pins instead
- **If multiple pins damaged**: ESP32 may need replacement

---

## Theory: Why This Circuit Works

### Voltage Divider Analysis

The circuit forms a voltage divider between the piezo and the ADC:

```
V_adc = V_piezo × (R2 / (R1 + R2))
      = V_piezo × (1kΩ / 1001kΩ)
      ≈ V_piezo × 0.001
```

Even if the piezo generates **40V**, the ADC sees:
- V_adc = 40V × 0.001 = **0.04V** (without diode clamping)

### Diode Clamping

If V_piezo > 3.3V:
- D1 conducts, clamping V_adc to 3.3V + 0.6V = **3.9V** (still safe)
- D2 prevents negative voltage

### Capacitor Filtering

C1 + R2 form a low-pass filter:
- Cutoff frequency: f_c = 1 / (2π × R2 × C1)
- With R2=1kΩ, C1=1µF: f_c = 159 Hz

This removes high-frequency noise while preserving the ~20-100Hz drum strike transient.

---

## Advanced: PCB Layout Recommendations

If designing a custom PCB:

### Layout Rules

1. **Short traces**: Keep ADC traces <10mm from protection circuit to ESP32 pin
2. **Ground plane**: Use solid ground plane on bottom layer
3. **Component placement**: Place protection circuit as close as possible to ADC pins
4. **Test points**: Add test points for each ADC signal for debugging
5. **Decoupling**: Add 10µF capacitor near ESP32 VCC pin

### Example Layout (Per Pad)

```
[Piezo In] ───┬─── [R1] ───┬─── [R2] ───┬─── [ADC Out]
              │             │            │
              GND           [D1]         [C1]
                            [D2]          │
                             │           GND
                            3.3V
```

---

## Safety Checklist

Before connecting to ESP32, verify:

- [ ] All components installed correctly
- [ ] Diode polarity verified (stripe = cathode)
- [ ] All 4 circuits tested with multimeter
- [ ] Maximum voltage NEVER exceeds 3.3V under hardest strike
- [ ] Idle voltage is 0V ± 0.05V
- [ ] All connections secure (no loose wires)
- [ ] ESP32 3.3V and GND rails connected to circuit
- [ ] Multimeter available for ongoing monitoring

---

## References

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) - Section 29: ADC
- [1N4148 Datasheet](https://www.vishay.com/docs/81857/1n4148.pdf)
- Application Note: "Piezo Element Conditioning for ADC Input" - Analog Devices

---

**Document Version**: 1.0
**Last Updated**: 2025-12-02
**Tested On**: ESP32-S3 DevKitC-1

**REMINDER**: Never connect piezos without protection circuits. When in doubt, test with multimeter first!
