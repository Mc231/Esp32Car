# Rover Command & Telemetry Protocol

**Version:** 1.0
**Status:** stable
**Last updated:** 2026-05-07

This document is the source of truth for the JSON message format the rover
firmware speaks across **all** transports (HTTP, WebSocket, Telnet, Serial,
ESP-NOW, MQTT). Messages are identical regardless of pipe — only framing
changes (HTTP body, WS frame, line-delimited on Telnet/Serial, magic-prefixed
binary on ESP-NOW, MQTT topic-routed).

---

## 1. Top-level envelopes

The firmware emits exactly three envelope shapes. Clients send one (commands).

| Direction | Envelope key | Meaning |
|-----------|--------------|---------|
| client → rover | `command` | Request the rover do something or read state. |
| rover → client | `response` | Successful read/write reply (carries data). |
| rover → client | `status` | Diagnostic / error message (string). |
| rover → client | `telemetry` | Periodic state push (no client request). |

A reply payload contains exactly one of `response` / `status`. Telemetry
pushes are wrapped in `{"telemetry": {...}}`.

Forward-compatibility: clients **must** ignore unknown top-level keys and
unknown nested fields.

---

## 2. Command envelope

```jsonc
{
  "command": "<command-name>",   // required, string
  "...":     <args>              // command-specific args
}
```

If `command` is missing, not a string, or unknown, the rover drops the
message **without replying** (no error envelope). Diagnostic logs go to
the local Serial / Telnet log channel.

If a known command's arguments are missing or wrong-type, the rover
replies with a `status` envelope explaining why.

---

## 3. Commands

### 3.1 Read-only

| Command | Args | Reply | Notes |
|---------|------|-------|-------|
| `system` | — | `{"response": {<system metrics>}}` | uptime, free heap, RSSI, etc. |
| `status` | — | `{"response": {"motor": ..., "system": ...}}` | combined motor + system state. |
| `config` | — | `{"response": {<config>}}` | build-time pin map + feature flags (camelCase keys). |
| `wifi` | — | `{"response": {"ssid": "...", "ip": "..."}}` | current Wi-Fi station details. |
| `motor_state` | — | `{"response": {<motor state>}}` | see § 4.1. |
| `distance` | — | `{"response": {"last_distance": <cm>}}` or `{"status": "...not built..."}` | only present when firmware built with `ROVER_FEATURE_DISTANCE`. `-1.0` = nothing within 80 cm. |
| `transports` | — | `{"response": [{"name":"...","running":true}, ...]}` | list of registered transports. |

### 3.2 Mutations

| Command | Args | Reply | Notes |
|---------|------|-------|-------|
| `set_motor` | `action` (int 0–2), `motor` (int 0–2) | motor_state response, or `{"status":"…"}` on bad args | `action`: 0=FORWARD, 1=BACKWARD, 2=STOP. `motor`: 0=LEFT, 1=RIGHT, 2=BOTH. |
| `set_motor_pwm` | `motor` (int 0–2), `pwm` (int 0–255) | motor_state response, or `{"status":"…"}` | PWM is clamped to 0–255 at the manager level. |
| `set_camera` | `frame_size` (int) | `{"status":"Camera frame size updated"}` or `{"status":"Camera not initialized"}` | `frame_size` is a `framesize_t` enum value from esp_camera. |
| `set_transport` | `name` (string), `enabled` (bool) | `{"response":{"name":"…","running":true}}` or `{"status":"unknown transport"}` | In-memory only. To persist boot defaults, edit `RoverApplicationConfig`. |
| `reboot` | — | `{"status":"rebooting"}` then device resets | reply is sent before the actual reboot. |
| `forget_wi_fi` | — | `{"status":"Forget WI-FI Success"}` then device reboots into AP/captive-portal mode | clears stored Wi-Fi creds. |

### 3.3 Argument validation

The dispatcher rejects:

- Non-string `command`
- Missing required argument(s) → `{"status":"… not passed"}` / `{"status":"… missing"}`
- Wrong-type required argument (e.g. `action` as a string) → `{"status":"… must be integers"}`

Rejections never call into the controller — failed commands are
side-effect-free.

---

## 4. Response payload shapes

### 4.1 motor state

```jsonc
{
  "lma":   0,    // last left  motor action (FORWARD=0, BACKWARD=1, STOP=2)
  "rma":   0,    // last right motor action
  "las":   2,    // last action's MotorSelection (LEFT=0, RIGHT=1, ALL=2)
  "lmpwm": 200,  // current left  PWM (0–255)
  "rmpwm": 200,  // current right PWM
  "lpwms": 2     // last setPWM's MotorSelection
}
```

### 4.2 system state

The keys `systemMonitor` populates depend on the build, but always include:

```jsonc
{
  "up_time":   12345,        // ms since boot
  "free_heap": 188432,       // bytes
  "rssi":      -56           // dBm, signed
}
```

### 4.3 distance state (gated on `ROVER_FEATURE_DISTANCE`)

```jsonc
{
  "last_distance": 35.2      // cm; -1.0 = nothing within 80cm
}
```

---

## 5. Telemetry envelope

The rover emits unsolicited telemetry on transports that support push
(WS, MQTT). Cadence is set by the transport (typical: 4 Hz).

```jsonc
{
  "telemetry": {
    "system":   { "up_time": 12345, "free_heap": 188432, "rssi": -56 },
    "motor":    { "lma": 0, "rma": 0, "lmpwm": 0, "rmpwm": 0, ... },
    "distance": { "last_distance": 35.2 }   // only if ROVER_FEATURE_DISTANCE
  }
}
```

---

## 6. Transport-specific framing

| Transport | Default port | Framing |
|-----------|--------------|---------|
| HTTP      | 32231        | `POST /api/cmd` with JSON body. Reply is a JSON body. |
| WebSocket | 32232        | One JSON object per text frame. |
| Telnet    | 23           | One JSON object per line (`\n`-delimited). Replies prefixed with `>>> `. Logs interleave on the same socket. |
| Serial    | UART0 (115200) | One JSON object per line. Replies prefixed with `>>> `. |
| ESP-NOW   | broadcast    | 4-byte magic prefix `RVC0` + JSON. Max payload **246 B** (250 B frame – 4 B prefix). |
| MQTT      | broker       | Topics: `rover/<id>/cmd` (sub), `rover/<id>/response`, `rover/<id>/telemetry`, `rover/<id>/status`. |

---

## 7. Versioning

This document is version `1.0`. Future versions:

- **Patch (1.0 → 1.1):** added optional fields, new commands, new
  response keys. Existing clients keep working.
- **Major (1.x → 2.0):** breaking changes to existing field names or
  semantics. Bump and document migration notes here.

A `protocol_version` field on the `system` response is planned for
clients that need to negotiate features at runtime — not yet shipped.

---

## 8. Quick reference for common interactions

```jsonc
// Forward both motors at full speed:
→ {"command":"set_motor_pwm", "motor":2, "pwm":255}
→ {"command":"set_motor",     "motor":2, "action":0}

// Stop:
→ {"command":"set_motor", "motor":2, "action":2}

// Read distance:
→ {"command":"distance"}
← {"response":{"last_distance": 35.2}}

// Rotate in place (left back, right forward):
→ {"command":"set_motor", "motor":0, "action":1}
→ {"command":"set_motor", "motor":1, "action":0}
```
