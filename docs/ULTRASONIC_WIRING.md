# Wiring HC-SR04 to ESP32-CAM (battery-powered, no soldering)

> ⚠️ **Historical / deprecated.** The project moved off the HC-SR04. The
> active distance-sensor wiring is documented in
> [`DISTANCE_SENSOR_WIRING.md`](./DISTANCE_SENSOR_WIRING.md) (Sharp GP2Y0A21).
> This file is kept only for reference if someone wants to revisit
> ultrasonic on the AI-Thinker board, where the two-pin scheme below
> still works.

This guide covers wiring an HC-SR04 ultrasonic distance sensor to an AI-Thinker ESP32-CAM when:

- You only have access to the **side rails** (no soldering to GPIO 33 on the back).
- Your motors take the **right-side pins** (IO 2 / 4 / 12 / 13 / 14 / 15).
- The sensor is powered from a **separate 5 V battery**, not from the ESP32.

---

## Why we need resistors

The HC-SR04's `ECHO` pin outputs **5 V**. ESP32 GPIOs accept **3.3 V max**. Without protection the chip slowly degrades.

The fix is a **voltage divider** — two resistors that turn 5 V into ~3.3 V.

```
ECHO (5V) ──[ 1 kΩ ]──┬── ESP32 GPIO  (≈3.3 V here)
                      │
                    [ 2 kΩ ]
                      │
                     GND
```

Math: `5 V × (2 kΩ / (1 kΩ + 2 kΩ)) = 3.33 V`. Safe.

`TRIG` doesn't need a resistor — ESP32 outputs 3.3 V and the HC-SR04 happily reads that as HIGH.

## Why a third resistor (the 10 kΩ pull-up)

We have to use **IO3 (RX)** for ECHO because it's the only second pin available. IO3 is also a boot strap pin — if it's LOW at power-on, the chip can refuse to boot.

The HC-SR04's ECHO sits LOW when idle. So at boot, the sensor would pull IO3 LOW and break the bootloader.

A **10 kΩ pull-up resistor** between IO3 and 3V3 fixes this: it's weak enough that the sensor wins when actually driving the line, but strong enough to hold IO3 HIGH during the brief boot window.

```
3V3 ──[ 10 kΩ ]──┬── IO3
                 │
ECHO ─[ 1 kΩ ]───┤
                 │
                [ 2 kΩ ]
                 │
                GND
```

---

## Bill of materials

| Qty | Part                         | Color code (4-band)                    |
| --- | ---------------------------- | -------------------------------------- |
| 1   | 1 kΩ resistor                | brown – black – red                    |
| 1   | 2 kΩ (or 2.2 kΩ) resistor    | red – black – red (or red – red – red) |
| 1   | 10 kΩ resistor               | brown – black – orange                 |
| 1   | HC-SR04 ultrasonic sensor    | —                                      |
| 1   | 5 V battery / power source   | see *Power notes* below                |
| —   | small breadboard + jumpers   | —                                      |

---

## Step-by-step wiring

I'll use breadboard-style notation: a **rail** is a horizontal strip on a breadboard where everything in that strip is electrically connected.

### Reserve 4 free rails on your breadboard

- Rail **A** — raw ECHO signal coming from the sensor
- Rail **B** — divided ECHO signal (this goes to the ESP32 IO3 + the pull-up)
- Rail **C** — ESP32 3V3
- Breadboard's **GND rail** (the long blue/black strip)

### Step 1 — Power and common ground

1. Wire **HC-SR04 VCC** → battery **+5 V**.
2. Wire **HC-SR04 GND** → breadboard **GND rail**.
3. Wire **battery −** → breadboard **GND rail**.
4. Wire **ESP32-CAM GND** (any of the three GND pins) → breadboard **GND rail**.

> **Critical:** battery negative, HC-SR04 GND, and ESP32 GND must all meet on the same GND rail. This is the *common ground*. Without it, the divider math is meaningless.

### Step 2 — TRIG (no resistor)

1. Wire **HC-SR04 TRIG** → **ESP32-CAM IO16**.

### Step 3 — ECHO with the voltage divider

1. Wire **HC-SR04 ECHO** → rail **A**.
2. Plug a **1 kΩ resistor** between rail **A** and rail **B**.
3. Plug a **2 kΩ resistor** between rail **B** and the **GND rail**.
4. Wire rail **B** → **ESP32-CAM IO3** (labelled `U0R` or `RX`).

### Step 4 — Boot-safe pull-up on IO3

1. Wire **ESP32-CAM 3V3** → rail **C**.
2. Plug a **10 kΩ resistor** between rail **C** and rail **B**.

### Final connection list (cross-check)

| From                 | Through    | To                   |
| -------------------- | ---------- | -------------------- |
| Battery +5 V         | wire       | HC-SR04 VCC          |
| Battery −            | wire       | GND rail             |
| HC-SR04 GND          | wire       | GND rail             |
| ESP32-CAM GND        | wire       | GND rail             |
| HC-SR04 TRIG         | wire       | ESP32-CAM IO16       |
| HC-SR04 ECHO         | wire       | rail A               |
| rail A               | **1 kΩ**   | rail B               |
| rail B               | **2 kΩ**   | GND rail             |
| rail B               | wire       | ESP32-CAM IO3 (RX)   |
| ESP32-CAM 3V3        | wire       | rail C               |
| rail C               | **10 kΩ**  | rail B               |

### Visual summary

```
            BATTERY (5 V)
             ┌─────┐
        +────┤     ├────−
        │              │
        │              ├──────────────────────────┐
        │              │                          │
        │      HC-SR04 │                          │   ESP32-CAM
        │      ┌───────┴┐                         │   ┌─────────┐
        ├──────┤ VCC    │                         │   │   3V3   ├──┐
        │      │ TRIG   ├─────────────────────────┼───┤ IO16    │  │
        │      │ ECHO   ├──── 1 kΩ ───────────────┼─┬─┤ IO3(RX) │  │
        │      │ GND    ├─────────────────────────┤ │ │ GND     ├──┼── (common GND wire)
        │      └────────┘                         │ │ └─────────┘  │
        │                                         │ │              │
        │                                       [2kΩ]            [10kΩ]
        │                                         │ │              │
        └─────────────────────────────────────────┘ └──────────────┘
                                                ↑
                                       same GND rail
```

---

## Power-on sequence (recommended)

1. Power the **ESP32-CAM first**. Wait for boot (captive portal or Wi-Fi LED activity).
2. Then power the **sensor's 5 V battery**.

This way, ECHO is in high-impedance (sensor unpowered) during ESP32 boot. The 10 kΩ pull-up cleanly holds IO3 HIGH and the chip boots reliably every time.

If both power on simultaneously, the pull-up usually still wins. If you ever see the rover refusing to boot, briefly unplug the sensor's battery, boot the ESP32, then reconnect.

---

## Power notes (battery side)

| Battery type      | What to do                                                                                  |
| ----------------- | ------------------------------------------------------------------------------------------- |
| 5 V power bank    | Plug straight into HC-SR04 VCC. ✅ ideal.                                                   |
| 1S LiPo (3.7 V)   | Too low. Use the **HC-SR04P** (3.3 V variant) instead — also lets you skip the divider.     |
| 2S LiPo (7.4 V)   | Too high — will fry the sensor. Use a **buck converter** (LM2596 / MP1584) to step down.    |
| 6–12 V motor pack | Same — needs a buck converter. Or tap the **L298N's onboard 5 V regulator output** if your driver has one. |

> If you use the L298N's 5 V output for the sensor, remember the L298N GND is already tied to ESP32 GND for motor control — common ground is already satisfied.

---

## Firmware change

Open `src/main.cpp` and replace the contents with:

```cpp
#include <Arduino.h>
#include <RoverApplication.h>

static RoverApplicationConfig makeConfig() {
  RoverApplicationConfig c;
  c.ultrasonicPin1          = 16;   // TRIG
  c.ultrasonicPin2          = 3;    // ECHO
  c.ultrasonicSensorEnabled = true;
  return c;
}

RoverApplication roverApp(makeConfig());

void setup() { roverApp.setup(); }
void loop()  { roverApp.loop(); }
```

Reflash. You'll lose Serial *input* (we're using IO3 for ECHO instead of UART RX), but Serial *output / logs* keeps working on IO1 (TX). The control panel's **Distance** overlay should now show centimetres.

---

## Trade-offs

- **Serial input** is gone. Logs still work; the rover just can't receive bytes from the host. For 99% of use, you don't care.
- **Boot reliability** is good but not perfect. The pull-up handles the common case; powering the sensor after the ESP32 makes it bulletproof.
- **5 V on a 3.3 V GPIO** if you ever forget the divider = slow chip damage. Always test continuity with a multimeter before powering on for the first time.

---

## Cleaner alternatives (for later)

1. **HC-SR04 single-pin mode** — TRIG and ECHO tied together through a 1 kΩ resistor, only one ESP32 GPIO (IO16) needed. Avoids the boot-strap problem entirely. Requires a small refactor of `UltrasonicManager`.

2. **HC-SR04P (3.3 V variant, ~$2)** — drops the voltage divider requirement. The 10 kΩ pull-up is still recommended.

3. **Solder a wire to GPIO 33** on the back of the ESP32-CAM module (the onboard red-LED pad). Then TRIG=IO16, ECHO=IO33 (still with divider). No serial loss, no boot drama. Cleanest if you can solder.

4. **VL53L0X / VL53L1X laser ToF sensor** — I2C, 3.3 V native. Better range and accuracy. Needs 2 free pins (SDA + SCL), which means option 3 (soldering) or freeing a motor pin first.
