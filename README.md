# ESP32-CAM Rover

A Wi-Fi controlled rover built on the AI-Thinker ESP32-CAM (and ESP32-WROVER variant). Streams video, drives differential-drive motors via an L298N (or compatible) driver, and exposes a unified JSON command surface across **HTTP, WebSocket, Telnet, Serial, ESP-NOW, and MQTT** transports. UI lives in the standalone `web/` app.

---

## Features

- **First-run captive portal** — connect to the rover's `Rover` Wi-Fi AP, pick your home network. Rover persists Wi-Fi creds and reboots into station mode. MQTT broker config lives in the build-time `RoverApplicationConfig` (see `src/main.cpp`).
- **Standalone web UI** at `web/` — camera stream, on-screen D-pad, keyboard arrow keys, speed slider, live telemetry overlays, multi-rover picker, recording + reverse-replay, autonomous obstacle-avoidance. Pure browser, no build step.
- **Camera streaming** — MJPEG from the OV2640 on its own server (port 81).
- **Unified JSON command surface** across six transports — same envelope (`{"command":"…"}`), same handlers, different pipes:
  - **HTTP** `POST /api/cmd` on port 32231
  - **WebSocket** on port 32232 (used by the standalone web app)
  - **Telnet** on port 23 — same connection serves live logs and accepts JSON commands; replies prefixed with `>>> `
  - **Serial** — JSON-per-line over USB UART, replies prefixed with `>>> `
  - **ESP-NOW** — peer-to-peer 2.4 GHz broadcast (no router needed); ~200 m line-of-sight; 4-byte magic prefix `RVC0` + JSON, 246 B max payload
  - **MQTT** publish/subscribe on `rover/<id>/cmd|response|telemetry|status` (opt-in env, configurable broker)
- **mDNS discovery** — reachable at `Rover.local`, advertises camera/control/WS services for zeroconf browsers.
- **HTTP basic auth** support (off by default) on the control HTTP server.

### Compile-time feature flags

Per-env flags pick what the firmware ships with. Set in `platformio.ini` build_flags:

| Flag | Effect |
| ---- | ------ |
| `ROVER_FEATURE_DISTANCE` | Enables the Sharp GP2Y0A21 IR distance sensor + the `distance` command. WROVER envs set this; AI-Thinker doesn't (no free pin). |
| `ROVER_FEATURE_MQTT`     | Pulls in `PubSubClient` and the MQTT bridge. |
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

## Using the rover

There is no firmware-served HTML — point any of the transports at the rover.

- **Browser:** open the standalone web app in `web/` over a local HTTP server (see `web/README.md`). Uses the WebSocket transport.
- **HTTP:** `curl -X POST http://Rover.local:32231/api/cmd -d '{"command":"system"}'`
- **WS:** `ws://Rover.local:32232/` — send a JSON frame, receive a JSON reply.
- **MQTT:** publish to `rover/<id>/cmd`, subscribe to `rover/<id>/response` and `rover/<id>/telemetry`.
- **Serial:** `pio device monitor`, type `{"command":"system"}` and press Enter. Reply lines are prefixed with `>>> ` so they're easy to grep out of log output.
- **Telnet:** `nc Rover.local 23` — see live logs streaming, type a JSON command + Enter, reply lines come back prefixed with `>>> `. Same connection, both directions.
- **ESP-NOW:** another ESP32 on the same Wi-Fi channel sends `[4-byte 'RVC0'][JSON]` to the broadcast MAC. Replies arrive as broadcast frames. Useful for a phone-free joystick remote or rover-to-rover coordination — no router, no IP.

---

## Command reference

All four transports accept the **same JSON envelope**. Body shape: `{"command": "<name>", ...args}`. Reply: `{"response": <object>}` or `{"status": "<msg>"}`.

| Command          | Args                                                | Reply / Effect                                |
| ---------------- | --------------------------------------------------- | --------------------------------------------- |
| `system`         | —                                                   | uptime, free heap                             |
| `status`         | —                                                   | motor state + system                          |
| `config`         | —                                                   | pin assignments                               |
| `wifi`           | —                                                   | SSID + IP (no password)                       |
| `forget_wi_fi`   | —                                                   | wipes Wi-Fi creds, reboots                    |
| `distance`       | —                                                   | last distance reading (cm)                    |
| `motor_state`    | —                                                   | current motor action + PWM per side           |
| `set_motor`      | `action` (0=fwd,1=back,2=stop), `motor` (0=L,1=R,2=both) | drives, replies with motor state         |
| `set_motor_pwm`  | `motor` (0/1/2), `pwm` (0-255 int)                  | applies PWM, replies with motor state         |
| `set_camera`     | `frame_size` (0-13)                                 | switches MJPEG resolution                     |
| `reboot`         | —                                                   | replies, then restarts                        |
| `transports`     | —                                                   | list of registered transports + running state |
| `set_transport`  | `name` ("http"/"ws"/"telnet"/"serial"/"espnow"/"mqtt"), `enabled` (bool) | start/stop a transport at runtime. In-memory only — boot defaults come from `RoverApplicationConfig`. |

**MQTT extras:** when MQTT is enabled, the rover also auto-publishes a combined telemetry frame to `rover/<id>/telemetry` every 2 s (no command needed) and a retained `online`/`offline` LWT on `rover/<id>/status`.

**HTTP extras:** beyond `/api/cmd`, the firmware HTTP server hosts utility pages:
- `GET /ota` + `POST /ota/upload` — over-the-air firmware update
- `GET /logs` + `GET /logs/data` — live log viewer

The MJPEG camera stream is on **port 81**.

---

## Configuration

All defaults live in [`lib/RoverApplication/src/Config/RoverApplicationConfig.h`](lib/RoverApplication/src/Config/RoverApplicationConfig.h). You can either edit them in place or override per-build in `src/main.cpp`:

```cpp
#include <Arduino.h>
#include <RoverApplication.h>

RoverApplicationConfig cfg;
cfg.apSsid          = "MyRover";
cfg.apPassword      = "supersecret";
cfg.adminUser       = "admin";              // enable HTTP basic auth
cfg.adminPassword   = "yourpass";           // empty = auth disabled
cfg.mqttEnabled     = true;                 // requires -DROVER_FEATURE_MQTT
cfg.mqttHost        = "192.168.1.10";
cfg.mqttUser        = "rover";
cfg.mqttPassword    = "secret";

RoverApplication roverApp(cfg);

void setup() { roverApp.setup(); }
void loop()  { roverApp.loop(); }
```

### Notable settings

| Field                         | Default               | Notes                                                  |
| ----------------------------- | --------------------- | ------------------------------------------------------ |
| `apSsid` / `apPassword`       | `Rover` / `123456789` | Setup-mode AP                                          |
| `webServerPort`               | `32231`               | HTTP `/api/cmd` + OTA + logs pages                     |
| `webSocketPort`               | `32232`               | WebSocket                                              |
| `mdnsDiscoveryName`           | `Rover`               | Resolves as `<name>.local`                             |
| `serialBaud`                  | `115200`              | Match your serial monitor                              |
| `distanceSensorPin`           | board-dependent       | Sharp GP2Y0A21 analog out. Compile with `-DROVER_FEATURE_DISTANCE` to enable. |
| `adminUser` / `adminPassword` | empty                 | HTTP basic auth on the control HTTP server; both empty = disabled |
| `mqttEnabled`                 | `false`               | MQTT bridge on/off. Only consulted when `-DROVER_FEATURE_MQTT` is set. |
| `mqttHost` / `mqttPort`       | empty / `1883`        | Broker address. Empty host disables MQTT regardless of `mqttEnabled`. |
| `mqttUser` / `mqttPassword`   | empty                 | Broker auth (optional)                                 |
| `mqttClientId`                | empty                 | Empty → derived from MAC at boot                       |
| `mqttTopicPrefix`             | empty                 | Empty → `"rover"`                                      |
| `deadmanTimeoutMs`            | `1500`                | Auto-stop motors if no driving command in this window. 0 = disabled. |

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
│   ├── Config/                     # Compile-time + runtime config (MQTT/BLE settings)
│   ├── Command/                    # CommandDispatcher — shared JSON command surface
│   ├── Camera/                     # OV2640 init, esp32-camera wrapper (port 81 stream)
│   ├── Control/                    # MotorControl + MotorManager
│   ├── Controller/                 # RoverController — central state, mutex-guarded
│   ├── Fs/                         # AbstractFS + SPIFFS impl
│   ├── Manager/                    # DistanceManager (Sharp GP2Y0A21 IR — ROVER_FEATURE_DISTANCE)
│   ├── Monitor/                    # SystemMonitor — uptime, free heap
│   ├── PostSetupBroadcaster/       # mDNS service broadcaster
│   ├── RoverWebServer/             # HTTP transport: /api/cmd + OTA + logs (port 32231)
│   ├── RoverWebSocket/             # WebSocket transport (port 32232)
│   ├── Serial/                     # Serial / USB UART transport (JSON per line)
│   ├── EspNow/                     # ESP-NOW peer-to-peer transport (broadcast)
│   ├── Mqtt/                       # MQTT bridge (opt-in, ROVER_FEATURE_MQTT)
│   └── WiFi/                       # Setup manager + captive portal HTML + persisted Wi-Fi creds
├── web/                            # Standalone browser app — the rover's UI
└── data/                           # Empty; reserved for future SPIFFS-served assets
```

### Key design choices

- **`RoverApplication` is a global** in `main.cpp`. Any code that needs FreeRTOS (e.g. `SPIFFS.begin(true)` formatting) must run from `setup()`, not from a constructor — `WiFiConfigManager` lazily mounts the FS for this reason.
- **`std::any` map** is used for telemetry payloads; `asJSON()` serializes via non-throwing pointer-form `std::any_cast<T>(&v)`.
- **Single mutex** (`SemaphoreHandle_t`) inside `RoverController` protects motor / distance state from races between transports.
- **Distance capture is polled** via `analogRead()` with oversampling — the Sharp GP2Y0A21 is an analog sensor.

---

## Tests

Two test suites — host-side, no flashing required.

### Web (vitest + jsdom)

```bash
cd web
npm install
npm test            # one-shot
npm run test:watch  # interactive
npm run coverage    # text + HTML report under coverage/
```

Covers `autonomous.js` (state machine), `recording.js` (replay engine),
`ws.js` (FIFO + reconnect + timeout), `store.js` (localStorage),
`toast.js` (DOM). 100% line coverage on those modules; **60 tests**.
Page-entry glue (`control.js`, `picker.js`, `controls-input.js`) is
DOM-orchestration code and intentionally not unit-tested.

### Firmware (PlatformIO + Unity, host-native)

```bash
pio test -e native
```

Compiles on the host (no flash, no device). Stubs in `test/native_stubs/`
provide host-friendly versions of `Arduino.h`, `RoverController`,
`SystemMonitor`, `RemoteLogger`, etc. so the tested code links without
FreeRTOS / Wi-Fi / camera deps.

Currently covers:
- `TransportRegistry` — add/find/loopAll/stopAll, idempotency
- `CommandDispatcher` — every command in the protocol, parser error
  paths, reply shape, transport-toggle commands

**36 tests**. Hardware-touching code (transport adapters, motor/distance
managers, camera, OTA) needs on-device tests for real coverage — those
aren't set up here.

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

1. Open `http://Rover.local:32231/ota`.
2. Pick a `firmware.bin` (e.g. `.pio/build/wrover-cam/firmware.bin`).
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
