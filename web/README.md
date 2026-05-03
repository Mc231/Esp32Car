# Standalone Rover Control web app

A small browser-only web app that discovers your rovers, lets you pick one,
and gives you the same camera + control panel that the firmware serves —
but hosted outside the rover.

The firmware's built-in control panel (served from the WROVER on
`http://Rover.local:32231/`) is **untouched**. This is an additional way to
control the rover, not a replacement.

---

## Why a separate app

- **Multi-rover.** Maintain a list of rovers, jump between them.
- **Saves rover flash.** Future feature additions don't have to fit in the
  ~150 KB the firmware page already takes.
- **Iterate on the UI without reflashing** — just edit and refresh.
- Lives in version control like normal web code.

---

## Files

```
web/
├── index.html      # Rover picker (list, add, remove, online status)
├── control.html    # Camera + d-pad + telemetry for one selected rover
└── README.md       # this file
```

No build step, no dependencies, no node_modules. Single-page apps using
plain HTML + vanilla JS. Open the source to see what they do.

---

## How to run

You can't open `index.html` directly with `file://` — modern browsers block
WebSocket connections from `file://` origins. You need a tiny local web
server. Pick whichever you have:

### Python (pre-installed on macOS)
```bash
cd web
python3 -m http.server 8000
```

### Node.js (if you have npm)
```bash
cd web
npx serve -l 8000 .
```

### Then open
```
http://localhost:8000/
```

---

## How it works

### Discovery (manual)

Browsers cannot do mDNS lookups or subnet scans for security reasons, so
auto-discovery isn't possible without a backend helper. Instead:

1. You add each rover by **IP address** (e.g. `192.168.1.42`) or **mDNS
   hostname** (e.g. `Rover.local` — works because the OS resolves it, the
   browser just sees a hostname).
2. The picker saves your list in `localStorage` so it persists across
   reloads.
3. Each rover row probes its `ws://<host>:32232/` WebSocket port to show
   an online/offline pill. Probes auto-refresh every 15 seconds.

### Control transport (WebSocket only)

The control page uses the rover's WebSocket server (`ws://<host>:32232/`)
for **all** commands — telemetry, motor control, reboot. We avoid HTTP
fetch() entirely so there are no CORS issues to worry about (the WROVER
doesn't set CORS headers, and we don't want to add them just for this).

Available WS commands the page sends:

| Command | What it does |
| --- | --- |
| `system` | uptime, free heap |
| `distance` | last reading from the GP2Y0A21 |
| `motor_state` | current motor action + PWM |
| `wifi` | rover's IP / SSID |
| `set_motor` | drive forward / back / stop |
| `set_motor_pwm` | set per-motor speed |
| `reboot` | restart the WROVER |

### Camera stream

Displayed as a plain `<img>` pointing at `http://<host>:81/stream` (the
ESP-Camera library's MJPEG endpoint, registered by `startCameraServer()`
in the firmware). Browsers render `multipart/x-mixed-replace` images
natively without any CORS check.

If the camera isn't streaming (or the rover is offline), the page shows a
"Camera stream unavailable" message and the rest of the controls keep
working over WebSocket.

---

## Limitations

- **No HTTPS hosting.** The WROVER's HTTP/WS endpoints are plain
  `http://` / `ws://` (no TLS). Browsers block "mixed content" — meaning
  if you host this page on `https://` (e.g. GitHub Pages), the WS and
  camera-stream connections will be blocked. Use a local HTTP server
  (the steps above) so the page is served on plain HTTP too.
- **No real auto-discovery.** See the discovery note above. If you want
  it later, the cleanest path is a tiny local helper (Node or Python,
  ~50 LOC) that does the network scan and feeds the picker via a
  localhost API.
- **Single-reply WS protocol.** The rover's WS server doesn't tag
  responses with a request id, so this client relies on the server
  replying to commands in FIFO order. It does today, but be aware if
  you add new server-side commands.

---

## Roadmap (rough)

- Optional Node helper for real LAN auto-discovery (mDNS via `bonjour-service` or `dnssd`).
- Camera resolution selector, OTA / logs links per rover.
- Autonomous "avoid obstacles" mode that drives the rover from the
  client side using the live distance feed (the existing rover firmware
  already broadcasts everything we need).
