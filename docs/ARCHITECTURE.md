# Architecture

This document is a high-level map of how the firmware is layered and how a
single client command travels from any of the six transports down to motor
or sensor I/O. For protocol details (command names, argument shapes,
response envelopes) see [`PROTOCOL.md`](PROTOCOL.md).

---

## 1. Layer diagram

```mermaid
flowchart TB
  subgraph Clients
    UI[web/ standalone app]
    CLI[curl / netcat / mqtt cli]
    Peer[Other ESP32 \(ESP-NOW\)]
  end

  subgraph Transports["Transports (one envelope, six pipes)"]
    HTTP[RoverWebServer<br/>HTTP :32231]
    WS[RoverWebSocketServer<br/>WS :32232]
    TEL[RemoteLogger / Telnet :23]
    SER[RoverSerialServer<br/>UART0]
    ESP[RoverEspNowServer<br/>2.4 GHz peer]
    MQTT[RoverMqttClient<br/>broker rover/&lt;id&gt;/*]
  end

  REG[TransportRegistry<br/>list + lifecycle]
  DISP[CommandDispatcher<br/>parse → route → reply]

  subgraph Domain
    CTRL[RoverController<br/>deadman + state aggregator]
    MC[MotorControl<br/>action / setPWM]
    DM[DistanceManager<br/>oversample + GP2Y0A21 curve]
    SM[SystemMonitor<br/>uptime / heap / RSSI]
  end

  subgraph HAL["Hardware abstractions"]
    LM[MotorManager L]
    RM[MotorManager R]
    LEDC[ESP32 LEDC channels 4/5]
    ADC[ESP32 ADC]
    CAM[esp_camera<br/>OV2640 / LEDC ch 0]
    WIFI[Wi-Fi / mDNS]
  end

  Clients -->|JSON| Transports
  Transports --> DISP
  Transports -.register.-> REG
  REG -.toggle.-> Transports
  DISP --> CTRL
  CTRL --> MC
  CTRL --> DM
  DISP --> SM
  MC --> LM
  MC --> RM
  LM --> LEDC
  RM --> LEDC
  DM --> ADC
  CAM -.LEDC ch 0.- LEDC
```

---

## 2. Layers

| Layer | Lives in | Purpose |
|-------|----------|---------|
| **Transport** | `lib/RoverApplication/src/{RoverWebServer,RoverWebSocket,Serial,EspNow,Mqtt,Log}` | Bytes ↔ JSON-per-message. Implements `ICommandTransport` (`begin`/`stop`/`isRunning`/`name`). |
| **Registry** | `Command/TransportRegistry.{h,cpp}` | Holds the transports the dispatcher knows about. Powers the `transports` and `set_transport` commands so any pipe can toggle any other. |
| **Dispatch** | `Command/CommandDispatcher.{h,cpp}` | Parses one JSON envelope, validates types, routes to a handler, serialises the reply. The reply callback is supplied by the calling transport (`std::function<void(const std::string&)>`). |
| **Domain** | `Controller/RoverController` + `Manager/*` + `Control/MotorControl` | Business logic: motor routing, distance smoothing, deadman timeout, system metrics. No I/O directly — uses HAL classes. |
| **HAL** | `Manager/MotorManager`, ESP-IDF / Arduino-ESP32 calls | Pin / LEDC / ADC. The only layer allowed to touch hardware. |

The dispatcher is **transport-agnostic**: every transport's read path
calls `dispatcher.dispatchRaw(payload, replyFn)` with the same signature.
Replies go back via the supplied callback, so a Telnet command's reply
goes back over Telnet, an MQTT command's reply goes back over MQTT,
without the dispatcher knowing which is which.

---

## 3. Lifecycle (single command)

1. **Bytes arrive** on a transport (e.g. WS frame, MQTT publish, Telnet
   line, ESP-NOW broadcast).
2. **Transport** strips its framing (e.g. ESP-NOW removes the 4-byte
   `RVC0` magic prefix), turning the frame into a `std::string` payload.
3. **Transport** calls `dispatcher.dispatchRaw(payload, replyFn)` with a
   reply callback that writes back over the same pipe.
4. **Dispatcher** parses JSON, validates the `command` field is a string,
   and routes to the matching handler. Unknown / malformed commands are
   silently dropped (logged to Serial/Telnet).
5. **Handler** validates argument types (`is_number_integer()` etc.),
   replies with `{"status": "..."}` if validation fails, otherwise calls
   into `RoverController` / `MotorControl` / `DistanceManager`.
6. **Domain** acts: motors update via `MotorManager`'s LEDC channel,
   distance pulled from cached oversampled reading, etc.
7. **Handler** serialises the new state via `serialize()` and invokes
   `respond(...)`, which writes back over the originating transport.

---

## 4. Concurrency model

- One main loop on core 1 runs `transports.loopAll()`,
  `otaManager.loop()`, `Log.loop()`, `carController.tickDeadman()`,
  `distanceManager.update()`. All transport reads happen here.
- The camera streamer (`startCameraServer()` from esp_camera) runs on
  its own httpd task on core 0. It does not interact with the
  CommandDispatcher — it serves MJPEG only.
- Wi-Fi modem sleep is **disabled** (`WiFi.setSleep(false)`) — the
  camera I2S DMA stalls if the modem sleeps mid-frame. The cost is
  ~30 mA extra continuous draw.
- `RoverController::tickDeadman()` is the safety net: if no motor
  command has arrived within `deadmanTimeoutMs`, it forces all motors
  to STOP. Every transport benefits from this without doing it itself.

---

## 5. Critical invariants

| Invariant | Where enforced | Why |
|-----------|----------------|-----|
| Camera owns LEDC channel **0** (XCLK 20 MHz). | `CameraManager::initialize` (esp_camera default). | Calling `analogWrite()` for motor PWM auto-allocates channel 0 → kills the camera. |
| Motors get LEDC channels **4 and 5**. | `RoverApplication.cpp` ctor: `leftMotor(... 4)`, `rightMotor(... 5)`. | Avoids the camera channel and gives each motor its own DMA-friendly slot. |
| Camera init **before** motor init. | `RoverApplication::setup()` order. | esp_camera's LEDC setup is destructive; once camera is up, motor channels are safe to attach. |
| Telnet (`Log` on port 23) is BOTH a log sink AND a command transport. | `RoverApplication::registerTransports()` — `Log.setDispatcher(&commandDispatcher)`. | Single connection serves logs and accepts JSON commands; replies prefixed with `>>>`. Saves a port. |
| MQTT requires both a build flag (`ROVER_FEATURE_MQTT`) AND non-empty `mqttHost`. | `RoverApplication::registerTransports()` guard. | Lets you ship the firmware with the bridge compiled in but inactive when no broker is configured. |

---

## 6. Adding a new transport

1. Implement `ICommandTransport` (begin/stop/isRunning/name). The
   `loop()` method is where you read and call `dispatcher.dispatchRaw()`.
2. Construct it in `RoverApplication`'s initializer list.
3. Register it in `RoverApplication::registerTransports()`:
   `transports.add(&yourTransport); yourTransport.begin();`
4. Optional: gate the registration behind a build flag so the transport
   is compiled out of constrained builds.

That's it — `transports` and `set_transport` commands automatically
recognise the new transport. No dispatcher changes needed.

---

## 7. Adding a new command

1. Add a handler method on `CommandDispatcher` (header + cpp).
2. Wire the routing in `dispatchRaw` (one `else if` line).
3. Validate args defensively — missing fields and wrong types must
   reply with `{"status": "..."}`, never throw / abort.
4. Document the command in [`PROTOCOL.md`](PROTOCOL.md).
5. Add tests in `test/test_command_dispatcher/test_main.cpp`.
