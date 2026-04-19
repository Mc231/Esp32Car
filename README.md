# ESP32-CAM Rover

A Wi-Fi controlled rover built on the AI-Thinker ESP32-CAM. Streams video, drives differential-drive motors via an L298N (or compatible) driver, and exposes a modern web control panel plus a JSON REST + WebSocket API.

---

## Features

- **First-run captive portal** — connect to the rover's `Rover` Wi-Fi AP, pick your home network from a polished dark-themed UI, and the rover persists credentials and reboots into station mode.
- **Modern control panel** — responsive (mobile + desktop), camera stream, on-screen D-pad, keyboard arrow keys, speed slider, live telemetry overlays, settings drawer with hardware info, network details, Forget Wi-Fi, and Reboot.
- **Camera streaming** — MJPEG stream from the OV2640 on its own server (port 80), framesize switchable from 96×96 to 1600×1200.
- **REST API** on port 32231 (control) and **WebSocket** on port 32232 (low-latency control + telemetry).
- **mDNS discovery** — reachable at `Rover.local`, advertises three services so any zeroconf browser sees the camera, control panel, and WebSocket separately.
- **HTTP basic auth** support (off by default) on the control panel.
- **Optional ultrasonic distance sensor** (HC-SR04) — disabled by default because the AI-Thinker board has very few free GPIOs.
- **Production-shaped architecture** — dependency injection via a config struct, mutex-protected shared state, interrupt-driven sensor capture, abstract filesystem interface for testability.

---

## Hardware

| Component                     | Notes                                                      |
| ----------------------------- | ---------------------------------------------------------- |
| AI-Thinker ESP32-CAM          | with OV2640 camera                                         |
| L298N (or DRV8833) motor driver | dual H-bridge                                            |
| 2× DC gear motors + chassis   | any small differential-drive base                          |
| 5 V power supply, **≥ 1 A**   | wall adapter, buck converter, or LiPo + DC-DC step-up      |
| 470 µF + 100 nF capacitors    | bulk + bypass across 5V/GND at the ESP32-CAM               |
| FTDI / USB-to-Serial adapter  | for flashing only (3.3 V logic, 5 V power pin)             |
| Optional: HC-SR04             | ultrasonic distance sensor (see “Adding Ultrasonic” below) |

### Pinout (defaults)

| Function          | GPIO     | Notes                                |
| ----------------- | -------- | ------------------------------------ |
| Left motor IN1    | IO2      |                                      |
| Left motor IN2    | IO14     |                                      |
| Left motor PWM    | IO4      | shares pin with onboard flash LED    |
| Right motor IN1   | IO15     |                                      |
| Right motor IN2   | IO13     |                                      |
| Right motor PWM   | IO12     |                                      |
| Ultrasonic TRIG   | IO16     | only used if `ultrasonicSensorEnabled = true` |
| Ultrasonic ECHO   | IO33     | onboard red LED pad — needs back-of-board solder access |
| Serial logs       | IO1 / IO3 | UART0 — leave free for debugging    |
| Camera            | many     | reserved by camera driver, do not touch |

All pins are configurable via `RoverApplicationConfig` (see *Configuration*).

### Wiring the L298N

```
ESP32-CAM IO2    →  L298N IN1   (left motor)
ESP32-CAM IO14   →  L298N IN2
ESP32-CAM IO4    →  L298N ENA   (PWM)
ESP32-CAM IO15   →  L298N IN3   (right motor)
ESP32-CAM IO13   →  L298N IN4
ESP32-CAM IO12   →  L298N ENB   (PWM)
ESP32-CAM GND    →  L298N GND   (must share ground)
ESP32-CAM 5V     →  L298N 5V    (logic supply, only if your L298N has a jumper for it)
Battery 6-12 V   →  L298N VS    (motor supply)
```

### Power

The ESP32-CAM brown-outs easily — the camera + Wi-Fi spike to ~700 mA. **Use a real 5 V / ≥ 1 A supply** and add a **470 µF electrolytic + 100 nF ceramic** in parallel across the 5V/GND pins. USB ports on laptops are usually fine for testing without motors; the moment motors run, switch to a separate supply.

---

## Software

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- An FTDI / USB-to-Serial adapter at 3.3 V logic level

### Build & flash

```sh
# Build
pio run

# Flash (board must be in download mode: IO0 → GND, then RESET)
pio run -t upload

# Open serial monitor (115200 baud)
pio device monitor
```

After upload, **disconnect IO0 from GND and press RESET** to boot into the application.

### First-time setup (captive portal)

1. Power on the rover.
2. From your phone or laptop, connect to the Wi-Fi network **`Rover`** (default password `123456789`).
3. A captive portal page should open automatically. If not, browse to `http://192.168.4.1`.
4. Select your home Wi-Fi from the scanned list, enter the password, tap **Connect**.
5. The rover saves the credentials, reboots, and connects to your home network.

### Finding the rover

Once on your home network the rover advertises itself via mDNS as **`Rover.local`**. From most operating systems:

- **macOS / iOS:** open `http://Rover.local:32231/`
- **Windows:** install Bonjour Print Services first, then same URL
- **Android:** use a service browser app (e.g. *Service Browser*); look for `_http._tcp` (camera, port 80), `_rover-ctrl._tcp` (port 32231), `_rover-ws._tcp` (port 32232)
- **Fallback:** check your router's DHCP client list for the `Rover` hostname

The serial log also prints the IP and mDNS URL right after Wi-Fi connects.

---

## Using the control panel

Open `http://Rover.local:32231/` (or `http://<rover-ip>:32231/`).

| Element            | What it does                                                  |
| ------------------ | ------------------------------------------------------------- |
| **Top bar**        | Online dot, IP, uptime, free heap                             |
| **Camera view**    | MJPEG stream from the rover                                   |
| **Overlay stats**  | Live distance, motor states, current PWM                      |
| **D-pad**          | Touch to drive — Forward / Back / Left / Right / Stop         |
| **Speed slider**   | Sets PWM (0–255) for both motors when you release             |
| **Keyboard**       | Arrow keys = drive, Spacebar = stop                           |
| **Side buttons**   | Camera resolution, quick reboot                               |
| **Settings drawer** (gear icon) | Camera resolution, telemetry interval, network info, hardware pin map, **Refresh Info**, **Forget Wi-Fi**, **Reboot Rover** |

The page sends `stop` on browser tab blur so the rover doesn't keep driving when you switch apps.

---

## API reference

All endpoints are served from the **control server on port 32231**. The camera MJPEG stream is on **port 80**.

| Method | Path             | Body                                   | Notes                                    |
| ------ | ---------------- | -------------------------------------- | ---------------------------------------- |
| GET    | `/`              | —                                      | Returns the control HTML page            |
| GET    | `/status`        | —                                      | Combined motor + ultrasonic + system     |
| GET    | `/motor`         | —                                      | Motor state                              |
| PUT    | `/motor`         | `{"action":<int>,"motor":<int>}`       | action: 0=fwd, 1=back, 2=stop. motor: 0=L, 1=R, 2=both |
| PUT    | `/motorPWM`      | `{"motor":<int>,"pwm":"<int>"}`        | PWM 0-255 (string)                       |
| GET    | `/ultrasonic`    | —                                      | Last distance reading (cm)               |
| PUT    | `/camera`        | `{"frame_size":<int>}`                 | 0–13, see HTML dropdown for sizes        |
| GET    | `/system`        | —                                      | Uptime + free heap                       |
| GET    | `/config`        | —                                      | Pin assignments                          |
| GET    | `/wifi`          | —                                      | SSID + IP (no password leaked)           |
| POST   | `/wifi/forget`   | —                                      | Wipes saved Wi-Fi creds and reboots      |
| POST   | `/reboot`        | —                                      | Reboots the rover                        |

WebSocket (`ws://Rover.local:32232`) accepts the same operations as JSON commands; see `RoverWebSocketServer.cpp` for the protocol.

---

## Configuration

All defaults live in [`lib/RoverApplication/src/Config/RoverApplicationConfig.h`](lib/RoverApplication/src/Config/RoverApplicationConfig.h). You can either edit them in place or override per-build in `src/main.cpp`:

```cpp
#include <Arduino.h>
#include <RoverApplication.h>

RoverApplicationConfig cfg;
cfg.apSsid                  = "MyRover";
cfg.apPassword              = "supersecret";
cfg.adminUser               = "admin";        // enable HTTP basic auth
cfg.adminPassword           = "yourpass";     // empty = auth disabled
cfg.ultrasonicSensorEnabled = true;
cfg.ultrasonicPin1          = 16;             // TRIG
cfg.ultrasonicPin2          = 33;             // ECHO

RoverApplication roverApp(cfg);

void setup() { roverApp.setup(); }
void loop()  { roverApp.loop(); }
```

### Notable settings

| Field                       | Default      | Notes                                              |
| --------------------------- | ------------ | -------------------------------------------------- |
| `apSsid` / `apPassword`     | `Rover` / `123456789` | Setup-mode AP                             |
| `webServerPort`             | `32231`      | Control panel + REST                               |
| `webSocketPort`             | `32232`      | WebSocket                                          |
| `mdnsDiscoveryName`         | `Rover`      | Resolves as `<name>.local`                         |
| `serialBaud`                | `115200`     | Match your serial monitor                          |
| `ultrasonicSensorEnabled`   | `false`      | Disabled by default — see *Adding Ultrasonic*      |
| `adminUser` / `adminPassword` | empty      | HTTP basic auth on REST; both empty = disabled     |

---

## Adding ultrasonic (HC-SR04)

The AI-Thinker ESP32-CAM has very few free GPIOs once the camera and motors are wired. **Realistic options:**

### Option A — Solder access to GPIO 33 (cleanest)
Tack a wire to the GPIO 33 pad on the back of the module (the onboard red LED pad). Then:
- TRIG → IO16 (left side, free)
- ECHO → IO33 (with voltage divider, see below)

Set in `main.cpp`:
```cpp
cfg.ultrasonicPin1 = 16;
cfg.ultrasonicPin2 = 33;
cfg.ultrasonicSensorEnabled = true;
```

### Option B — HC-SR04 single-pin mode (no soldering, planned)
Wire TRIG and ECHO together through a 1 kΩ resistor, use only IO16. Requires a small refactor of `UltrasonicManager` (planned, not yet implemented).

### Voltage divider on ECHO (always required for HC-SR04)
The HC-SR04's ECHO pin outputs 5 V; ESP32 GPIOs are 3.3 V max:
```
HC-SR04 ECHO ──[ 1 kΩ ]──┬── ESP32 ECHO pin
                          │
                        [ 2 kΩ ]
                          │
                         GND
```

Skip the divider only if you're using the **HC-SR04P** (3.3 V variant).

### Avoid
- IO0 — boot strap pin, must be HIGH at boot.
- IO1 / IO3 — UART0 RX/TX. Using them kills serial logs and can fail boot when the HC-SR04's idle-LOW ECHO holds the strap pin.

---

## Project structure

```
Esp32Car/
├── platformio.ini
├── partitions_example.csv          # 2 MB app + 1 MB SPIFFS partition layout
├── src/
│   └── main.cpp                    # 12 lines — instantiates RoverApplication
├── lib/RoverApplication/src/       # All application logic lives here as a library
│   ├── RoverApplication.{h,cpp}    # Wires everything together
│   ├── Config/                     # Compile-time configuration struct
│   ├── Camera/                     # OV2640 init, esp32-camera wrapper
│   ├── Control/                    # MotorControl + MotorManager
│   ├── Controller/                 # RoverController — central state, mutex-guarded
│   ├── Fs/                         # AbstractFS + SPIFFS impl
│   ├── Html/                       # control_html.h (PROGMEM string)
│   ├── Manager/                    # UltrasonicManager (interrupt-driven)
│   ├── Monitor/                    # SystemMonitor — uptime, free heap
│   ├── PostSetupBroadcaster/       # mDNS service broadcaster
│   ├── RoverWebServer/             # REST control on port 32231
│   ├── RoverWebSocket/             # WebSocket on port 32232
│   └── WiFi/                       # Setup manager + captive portal HTML + persisted config
└── data/                           # Empty; reserved for future SPIFFS-served assets
```

### Key design choices

- **`RoverApplication` is a global** in `main.cpp`. Any code that needs FreeRTOS (e.g. `SPIFFS.begin(true)` formatting) must run from `setup()`, not from a constructor — `WiFiConfigManager` lazily mounts the FS for this reason.
- **`std::any` map** is used for telemetry payloads; `asJSON()` serializes via non-throwing pointer-form `std::any_cast<T>(&v)`.
- **Single mutex** (`SemaphoreHandle_t`) inside `RoverController` protects motor / ultrasonic state from races between HTTP and WebSocket handlers.
- **Ultrasonic capture is interrupt-driven** with an atomic ready flag; main loop calls `update()` and reads cached distance.

---

## Troubleshooting

| Symptom                                             | Likely cause / fix                                                                                                              |
| --------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `Failed to connect to ESP32: No serial data received` during `pio run -t upload` | Board not in download mode → tie IO0 to GND, press RESET, then upload. Check FTDI TX↔RX are crossed. |
| `waiting for download` shown after RESET            | IO0 is still tied to GND. Disconnect it and press RESET again.                                                                   |
| No serial logs after Wi-Fi connects                 | Something is using IO1 or IO3 (e.g. ultrasonic on UART pins). Move sensor pins.                                                  |
| Rover AP `Rover` not visible                        | Power supply too weak (use ≥ 1 A 5 V), check antenna selector solder blob is on the PCB-antenna pad, scan on 2.4 GHz.            |
| Captive portal shows empty network list             | Already fixed — `handleScanNetworks()` now disconnects STA before scanning. If you still see empty, your home Wi-Fi is 5 GHz only. |
| `SPIFFS: mount failed, -10025` once on first boot   | First-boot only; SPIFFS auto-formats. Should disappear on the next reboot.                                                       |
| `assert failed: xTaskGetIdleTaskHandleForCPU`       | Something is calling FreeRTOS APIs at static-init time. Defer to `setup()`.                                                      |
| Wi-Fi works on FTDI but disconnects on real 5 V supply | The ultrasonic on UART pins is interfering. Move it off IO1/IO3 or set `ultrasonicSensorEnabled = false`.                     |
| `_handleRequest(): request handler not found` in the captive portal log | Cosmetic — iOS/Android probing captive-detection URLs. Already redirected to `/`.                          |

---

## OTA updates

The rover supports two OTA paths after the **first** serial flash with the new partition layout (which has two app slots instead of one).

### Web OTA (end-user friendly)

1. Open `http://Rover.local:32231/ota` (or the gear icon → **Update Firmware (OTA)** button in the control panel).
2. Pick a `firmware.bin` (e.g. `.pio/build/esp32cam/firmware.bin`).
3. Click **Upload & Flash**. Progress bar shows the transfer; the rover reboots into the new firmware on completion.

If `adminPassword` is set in your config, the upload page is gated by HTTP basic auth.

### PlatformIO OTA (developer workflow)

A second build environment is defined in `platformio.ini`:

```sh
pio run -e ota -t upload
```

It uploads over Wi-Fi to `Rover.local` instead of via the FTDI cable. If you set `adminPassword`, uncomment the `upload_flags = --auth=YOUR_PASSWORD` line in `platformio.ini`.

### First time switching to OTA partitions

Because the partition layout changes (one big app slot → two OTA slots + smaller SPIFFS), the **first flash with the new layout must go over serial** and **erases SPIFFS** (you'll re-do the captive-portal Wi-Fi setup once). After that, all future updates can go over the air without losing config.

If you want a clean wipe, run:

```sh
pio run -t erase   # erases the whole flash, then upload normally
pio run -t upload
```

### OTA partition layout

See [`partitions_ota.csv`](partitions_ota.csv): two 1.9 MB app slots (`ota_0`, `ota_1`), a small `otadata` partition that tracks which slot is active, plus 192 KB SPIFFS. Current binary is ~1.8 MB, so OTA updates have ~150 KB of headroom before they outgrow the slot.

---

## Future work

- **Single-pin HC-SR04 driver** for AI-Thinker boards without soldering access to IO33.
- **WebSocket auth** (token in first frame).
- **Auth on camera server** (port 80) — currently unauthenticated.
- **Replace `std::any` payloads with `std::variant`** for type-safe telemetry without RTTI.
- **Move HTML assets into SPIFFS** so they can be updated without reflashing.
- **Rollback / health check** for OTA — currently a bad update bricks until next serial flash.
- **Unit tests** — abstract interfaces are already in place (`AbstractFS`, `AbstractWiFiSetupManager`).

---

## License

Personal project — no license specified. Add one if you intend to share.
