# Wiring an MPU6050 IMU (gyro + accelerometer) to the WROVER-CAM

The MPU6050 is a 6-DoF IMU (3-axis gyro + 3-axis accelerometer + temperature)
on an I²C bus. Adding it lets us track the rover's **heading**, detect
**tilt / fall** and **collisions**, and eventually replace the timing-based
reverse-replay with one that actually unwinds the heading you came from.

---

## Bill of materials

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | **MPU6050** breakout board (GY-521) | sold for ~$2 on Rozetka / AliExpress. Has pull-ups + voltage regulator on board. |
| 4 | jumper wires | female-to-female works directly into the WROVER's male headers. |

That's it. Pull-up resistors are on the GY-521 breakout already, so you don't
need any externally.

---

## The pin-budget problem

The WROVER-CAM has **no truly-free GPIOs** after the camera, motors, and
the existing GP2Y0A21 distance sensor. Adding I²C means **giving up
something**. The least-bad sacrifice is **Serial RX (IO3)**, which we
never use — we only ever *write* to Serial for boot logs (TX = IO1),
never read from it. Telnet on port 23 still works for any post-boot
diagnostics.

So the I²C pins become:
- **SDA → IO0** (also the BOOT button pin)
- **SCL → IO3** (was Serial RX)

### Why IO0 is OK as SDA

IO0 is a **strapping pin** — it must be HIGH at boot, otherwise the ESP32
enters Download mode. Normally that's a problem for a GPIO… but I²C lines
**idle HIGH** (they're driven LOW only during a transfer, by either side
of the bus). The MPU6050 breakout board has **4.7 kΩ pull-up resistors**
on SDA and SCL, so the line sits HIGH at boot exactly as required.

The on-board BOOT button (which forces IO0 LOW when pressed) still works
the same — it's just a button, no daily impact.

### Why we don't lose much by giving up Serial RX

- Serial **TX (IO1)** still prints all boot logs to USB. Untouched.
- Serial **RX (IO3)** is only used to receive bytes from the host over USB.
  Our firmware never reads from it. Sacrificing it costs us nothing.

---

## Wiring

```
                         ┌────────────────────────┐
                         │       BATTERY          │
                         │   +              −     │
                         └───┬──────────────┬─────┘
                             │              │
              ┌──────────────┤              ├──────────────┐
              │              │              │              │
              ▼              ▼              ▼              ▼
       ┌──────────┐   ┌──────────┐    ┌────────┐    ┌──────────┐
       │  WROVER  │   │  MPU6050 │    │ WROVER │    │  MPU6050 │
       │   3V3    │   │   VCC    │    │  GND   │    │   GND    │
       └──────────┘   └──────────┘    └────────┘    └──────────┘

       (existing GP2Y0A21, motors, camera all stay wired as-is)

                  ┌─────────────────────┐
                  │       MPU6050       │
                  │                     │
                  │   SDA ●─────────────┼──► WROVER  IO0
                  │   SCL ●─────────────┼──► WROVER  IO3
                  │                     │
                  │   AD0   ○ leave NC  │   (or to GND — sets I²C addr)
                  │   INT   ○ leave NC  │   (motion-interrupt — not used yet)
                  │   XDA / XCL  unused │
                  └─────────────────────┘
```

### Connection list

| From | To |
| ---- | -- |
| MPU6050 `VCC` | WROVER `3V3` (the breakout board's regulator handles 3.3 → 1.8 V internally) |
| MPU6050 `GND` | Common GND (same node as battery −, WROVER GND, GP2Y0A21 GND) |
| MPU6050 `SDA` | WROVER **IO0** |
| MPU6050 `SCL` | WROVER **IO3** |
| MPU6050 `AD0` | leave disconnected → I²C address `0x68`. Tie to GND for the same. Tie to 3V3 only if you want `0x69` (e.g. running two MPU6050s — not our case). |
| MPU6050 `INT` | leave disconnected for now. Optional later for motion-wake interrupts. |

> **Common ground rule still applies.** Battery −, WROVER GND, GP2Y0A21 GND,
> and MPU6050 GND must all meet on one node.

---

## Power notes

- **VCC: 3.3 V**, not 5 V. The MCU itself is 3.3 V; powering the breakout
  from 3V3 keeps SDA/SCL voltage levels safe with the ESP32. (The GY-521
  breakout has a 3.3 V LDO that accepts both 3.3 and 5 V, but feeding
  it from 3V3 is simpler.)
- The MPU6050 draws ~3.6 mA in normal operation — negligible compared to
  the camera or motors.
- Unlike the GP2Y0A21, the MPU6050 has **no current spikes**, so no
  decoupling cap is required.

---

## Firmware

A new manager class `ImuManager` lives at
`lib/RoverApplication/src/Manager/ImuManager.{h,cpp}`. It talks to the
MPU6050 directly via the Arduino `Wire` library — no extra dependency
in `platformio.ini`. The pins come from
`RoverApplicationConfig::imuSdaPin` and `imuSclPin` (default 0 and 3
on the WROVER build). The sensor is **disabled by default**; flip
`imuSensorEnabled = true` once the MPU6050 is wired up.

Once enabled, the rover broadcasts the IMU's raw axes via the existing
WebSocket server — request `{"command":"imu"}` to receive
`{"response":{"ax":…, "ay":…, "az":…, "gx":…, "gy":…, "gz":…, "heading":…, "temp":…}}`
where `ax/ay/az` are accelerometer in g, `gx/gy/gz` are gyro in deg/s,
`heading` is integrated yaw in degrees (resets on boot), and `temp` is
the chip temperature in °C. The standalone web app's control panel
will pick this up automatically.

---

## Future: heading-aware reverse-replay

The current reverse-replay is purely timing-based — replay each captured
move backwards for the same duration and hope the floor is flat / no
slip. With the IMU online, we can do:

- **Forward pass:** record `{action, durationMs, headingDelta}` per move.
- **Reverse pass:** for each move, drive the inverse direction until the
  heading delta is unwound (instead of just running for the same duration).

This handles slip, heading drift, and uneven surfaces much better than
time-based replay. Not implemented yet — when you've got the IMU wired,
ping the firmware code to add it.
