# Wiring the Sharp GP2Y0A21 IR distance sensor (WROVER-CAM)

This is the active distance-sensor wiring for the rover. The earlier
`ULTRASONIC_WIRING.md` covers the HC-SR04 attempt and is kept only as
historical reference — it didn't end up working in single-pin mode with
the user's specific HC-SR04 module.

---

## Why this sensor

| | HC-SR04 (previous attempt) | **Sharp GP2Y0A21YK0F** |
| --- | --- | --- |
| Range | 2 cm – 4 m | **10 cm – 80 cm** (perfect for rover obstacle avoidance) |
| Output | Pulse-width (digital) | **Analog voltage** (0.4 V at 80 cm, ~3 V at 10 cm) |
| Voltage | 5 V — needs divider for echo back to 3.3 V GPIO | 5 V — output already fits inside ESP32 ADC range |
| Pins on the WROVER side | 2 (or 1 with a Schottky diode) | **1** |
| Wiring complexity | Resistor divider, common ground, possibly a pull-up | **3 wires, no passives** |

The GP2Y0A21's 10–80 cm range is exactly the band you want for
"rover, slow down / turn before you hit something."

---

## Bill of materials

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | Sharp **GP2Y0A21YK0F** + JST 3-pin pigtail | sold as a unit; the cable usually has red / black / yellow wires |
| 1 | **10 µF electrolytic capacitor** (or larger, up to ~470 µF) | strongly recommended — see "Cold-boot brownout" below. Anything you have lying around in that range works. |

That's it. **No resistors, no diodes, no breadboard.**

---

## Wiring

```
                        ┌──────────────────────┐
                        │       BATTERY        │
                        │   +              −   │
                        └───┬──────────────┬───┘
                            │              │
                  ┌─────────┤              ├──────────┐
                  │         │              │          │
                  │      ┌──┴──┐         ┌─┴─┐        │
                  │      │ 5V  │         │GND│ ← WROVER-DEV header
                  │      └─────┘         └───┘        │
                  │                                   │
            (sensor red)                       (sensor black)
                  │                                   │
                  ▼                                   ▼
              ┌─────────────────────────────────────────┐
              │  Sharp GP2Y0A21YK0F                     │
              │                                         │
              │   VCC (red) ●                           │
              │   GND (black) ●                         │
              │   Vo  (yellow) ●─────► WROVER IO33      │
              │                                         │
              │     [≈ 10 µF cap across VCC and GND]    │
              └─────────────────────────────────────────┘
```

### Connection list

| From | To |
| ---- | -- |
| Battery `+5 V` | WROVER `5V` pin |
| Battery `+5 V` | Sensor red wire (VCC) |
| Battery `−` | WROVER `GND` |
| Battery `−` | Sensor black wire (GND) |
| Sensor yellow wire (Vo) | WROVER **IO33** |
| **10 µF cap +** | Sensor red wire / VCC pin |
| **10 µF cap −** | Sensor black wire / GND pin |

> **Common ground is required.** Battery `−`, WROVER `GND`, sensor `GND`, and the cap's negative leg must all meet on one electrical node.

### Sensor pin order (verify with silkscreen)

Wire colors aren't standardized across manufacturers. The truth is the
**silkscreen text printed on the small PCB next to the JST connector**:

```
Vo    GND    Vcc
```

Trace each wire colour back to the labelled pin it plugs into.
On the standard Pololu / clone cable: red = Vcc, black = GND, yellow = Vo.

---

## Why no resistors are needed

- The sensor's output (Vo) sits between **0.25 V (>80 cm) and ~3.1 V (10 cm)** — already within the ESP32 ADC's 3.3 V input range.
- IO33 is **ADC1_CH5**, which is ADC1 (works alongside Wi-Fi — ADC2 doesn't).
- IO33 is not a strapping pin, so wiring it has no effect on boot.

---

## Cold-boot brownout (known issue, has a fix)

### Symptom

When you power the rover from a battery for the **first time** (cold start),
the rover sometimes hangs and never connects to Wi-Fi. **Pressing the
RESET button** on the WROVER makes it boot normally and connect.

### Cause

The GP2Y0A21 draws short **~250 mA spikes** every 38 ms when it pulses its
IR LED. At cold boot, these spikes overlap with:

- Camera initialisation (high transient current)
- Wi-Fi radio start-up
- ESP32 brownout-detector arming

The shared 5 V rail dips low enough to trip the **brownout detector**, which
silently resets the chip — sometimes into a bad state where it fails to
connect to Wi-Fi. Pressing RESET after the rail has stabilised works because
the load profile is different (camera is already up, sensor is in steady
state).

### Confirming the diagnosis

**Disconnect the GP2Y0A21 (just unplug the 3 wires) and try a cold boot.**
If it boots cleanly without the sensor, the sensor's current draw is the
cause. (You will not get distance readings in this state, of course.)

### Fixes (in order of simplicity)

1. **Add a 10 µF (or larger) capacitor across the sensor's VCC and GND.**
   - Sharp's own datasheet recommends this. Cap absorbs the spikes locally
     so they never reach the 5 V rail.
   - Place the cap **as close to the sensor body as possible** — clip it
     across the red and black wires right at the sensor end, not at the
     battery end.
   - Polarity: long leg → red (positive), short leg → black (negative).
   - Anything from 10 µF to 470 µF works. Bigger is better, no upper
     practical limit.

2. **Add a bulk capacitor (100 µF+) on the WROVER's 5 V rail.**
   - Same idea but on the supply side. Less effective than (1) because it
     doesn't stop the spike at the source, but useful if you only have
     caps on hand for one location.

3. **Power the sensor from a separate 5 V supply.**
   - Cleanest solution but needs an extra battery / step-down regulator.
   - The two grounds **must still be tied together** (common ground required).

4. **Software workaround: defer camera init until after Wi-Fi is up.**
   - Sequences the boot so the camera and sensor never spike at the same
     time. Trades cold-boot reliability for ~2 s extra delay before the
     camera stream becomes available. Not implemented yet — ask if you
     want this rather than the cap.

### Recommended

**Option 1.** A 10 µF electrolytic capacitor is essentially free (a small
assortment kit is ~₴30–50 on Rozetka). It's the textbook fix and removes
the brownout entirely.

---

## Firmware

The firmware is already in place and reads this sensor by default on the
WROVER-CAM build:

- Manager class: `DistanceManager` (`lib/RoverApplication/src/Manager/DistanceManager.{h,cpp}`)
- Config field: `distanceSensorPin = 33`, `distanceSensorEnabled = true`
- WebSocket command: `"distance"`
- HTTP route: `GET /distance`
- HTTP `/status` field: `distance.last_distance` (in cm)
- Control panel UI label: "Distance sensor"

The `DistanceManager` reads the analog pin 8× per cycle (oversampling) at
~20 Hz, applies the empirical curve `distance_cm = 27.86 × V^-1.15`, and
exposes:

- `> 0` cm — valid distance reading
- `-1` cm — out of range (nothing within 80 cm)
- `10` cm — clamped (object inside the sensor's 10 cm blind zone)

No code changes required when you add the cap.
