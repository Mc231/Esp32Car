# Battery voltage monitoring

Goal: surface the battery voltage in the rover's web UI so we know when
to charge before the rover browns out (we already know brownout-on-load
is a real thing — see `DISTANCE_SENSOR_WIRING.md`).

---

## The pin-budget problem (and the clean fix)

The straightforward way to monitor battery is a **voltage divider into
an ADC pin**. Unfortunately, the ESP32's "WiFi-safe" ADC bank (ADC1) is
fully consumed on the WROVER-CAM:

| ADC1 channel | GPIO | Used by |
| --- | --- | --- |
| ADC1_CH0 | IO36 (SVP) | camera |
| ADC1_CH3 | IO39 (SVN) | camera |
| ADC1_CH4 | IO32 | left motor PWM |
| ADC1_CH5 | IO33 | GP2Y0A21 distance sensor |
| ADC1_CH6 | IO34 | camera (input-only) |
| ADC1_CH7 | IO35 | camera (input-only) |

ADC2 pins (IO0/2/4/12-15/25-27) **cannot be read while WiFi is active**
on ESP32 — they share the same ADC peripheral that the WiFi RF needs.
So we can't put battery monitoring directly on a free GPIO.

### The clean fix: ADS1115 on the I²C bus

The same I²C bus the [IMU wiring guide](./IMU_WIRING.md) sets up
(SDA=IO0, SCL=IO3) can host **multiple devices**. Adding an
**ADS1115 4-channel 16-bit ADC breakout** (~$2) gives us four
analog inputs with no extra GPIO cost and far better resolution
than the ESP32's built-in ADC anyway (16-bit vs 12-bit, with much
better linearity).

Same I²C bus → same SDA/SCL wires. ADS1115 default address is `0x48`,
MPU6050 is `0x68`, so they coexist with no conflict.

---

## Bill of materials

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | **ADS1115** breakout | ~$2 on Rozetka / AliExpress. 16-bit, 4 single-ended channels (or 2 differential). |
| 1 | **Resistor — top of divider** | depends on battery, see "Sizing the divider" below |
| 1 | **Resistor — bottom of divider** | same |
| 1 | (optional) 0.1 µF ceramic cap | across the ADC input → smooths noisy readings |
| — | hookup wire / breadboard / common ground | — |

The MPU6050 from `IMU_WIRING.md` is a hard prerequisite — the I²C bus is
shared.

---

## Sizing the divider

The ADS1115 with the default ±4.096 V full-scale gain accepts up to
4 V on its inputs — anything above can damage it. We pick R_top : R_bottom
so the divider scales the **maximum expected battery voltage** down to
≤ 3.0 V (leaves headroom for measurement error and battery surge).

| Battery type | Nominal | Max | Suggested divider | Reads back |
| --- | --- | --- | --- | --- |
| 1S LiPo (3.7 V) | 3.7 V | 4.2 V | 100 kΩ : 100 kΩ (÷2) | up to 2.1 V |
| 2S LiPo (7.4 V) | 7.4 V | 8.4 V | 220 kΩ : 100 kΩ (÷3.2) | up to 2.6 V |
| 3S LiPo (11.1 V) | 11.1 V | 12.6 V | 470 kΩ : 100 kΩ (÷5.7) | up to 2.2 V |
| 4× AA NiMH (4.8 V) | 4.8 V | 5.5 V | 100 kΩ : 100 kΩ (÷2) | up to 2.75 V |
| 5 V power bank | 5.0 V | 5.2 V | 100 kΩ : 100 kΩ (÷2) | up to 2.6 V |

Use **high-value resistors** (100 kΩ and up) so the divider draws
microamps from the battery — the divider running 24/7 will otherwise
waste a noticeable fraction of your battery.

### Math

```
V_adc = V_battery × R_bottom / (R_top + R_bottom)
```

Then in firmware, recover the battery voltage:

```
V_battery = V_adc × (R_top + R_bottom) / R_bottom
```

Use **the actual measured resistor values** (multimeter your two resistors)
for best accuracy — 1% resistors have ±1% tolerance; 5% can be off by
much more.

---

## Wiring

```
        BATTERY +────────┬─────────► (rest of the rover power chain)
                         │
                         │
                       [R_top]      ← e.g. 100 kΩ
                         │
                         ├──────────► ADS1115 A0
                         │
                       [R_bottom]   ← e.g. 100 kΩ
                         │
        BATTERY −────────┴────────── common GND
                         │
                         ├──────────► ADS1115 GND
                         │
                         ├──────────► WROVER GND
                         │
                         ├──────────► MPU6050 GND
                         │
                         └──────────► (every other GND)
```

Plus the I²C connections (shared with the MPU6050):

| ADS1115 pin | To |
| --- | --- |
| `VDD` | WROVER `3V3` |
| `GND` | common GND |
| `SDA` | WROVER `IO0` (same wire as MPU6050 SDA) |
| `SCL` | WROVER `IO3` (same wire as MPU6050 SCL) |
| `ADDR` | leave NC or to GND → I²C address `0x48` |
| `A0`  | divider midpoint (this is the battery sense input) |
| `A1`–`A3` | leave free for future sensors |

The ADS1115 already has internal pull-ups; the MPU6050 breakout has
external 4.7 kΩ pull-ups. They coexist fine — the line is still cleanly
pulled high.

---

## Firmware (planned, not implemented yet)

A `BatteryMonitor` manager class would:

1. Use `Wire` (already brought up by `ImuManager` if both enabled).
2. Sample ADS1115 channel A0 every ~1 s.
3. Apply the divider math + a configurable per-rover calibration scalar.
4. Surface state via the existing WS server as `{"command":"battery"}`
   returning `{"voltage": 3.92, "percent": 78, "low": false}`.

The web app would render a battery pill in the header next to the
existing Up / Heap pills, going amber under 30 % and red under 10 %.

This work is **not implemented yet** — the doc is here so the wiring
plan is captured. Ping the firmware code to add it once the ADS1115 is
physically wired up alongside the MPU6050.

---

## Why not skip the ADS1115 and read directly?

You technically *can* do it without an ADS1115 by:

1. **Briefly disabling WiFi** to read an ADC2 pin → ugly, drops connections, unreliable.
2. **Sacrificing the GP2Y0A21** and using IO33 for battery → no obstacle distance.
3. **Hijacking IO34 / IO35** by giving up the camera → no camera.

All three are worse than spending $2 on an ADS1115 that sits on a bus
you're adding for the IMU anyway. The ADS1115 also unlocks **three more
analog inputs** for free (A1/A2/A3) — plenty of room for future sensors
like a current sensor on the motor rail, ambient light sensor, etc.
