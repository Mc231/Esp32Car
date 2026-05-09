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
├── index.html              # Rover picker (HTML shell)
├── control.html            # Control panel (HTML shell)
├── README.md
├── css/
│   ├── theme.css           # design tokens (colours, spacing, type)
│   ├── components.css      # buttons, pills, toast, cards, logo
│   ├── picker.css          # picker page layout
│   └── control.css         # control page layout (header + camera + d-pad)
└── js/
    ├── store.js            # localStorage-backed rover list
    ├── toast.js            # tiny toast helper
    ├── ws.js               # WebSocket transport (queue, reconnect, probe)
    ├── recording.js        # record + reverse-replay engine
    ├── autonomous.js       # obstacle-avoidance state machine
    ├── controls-input.js   # d-pad / keyboard event bindings
    ├── transports.js       # firmware transport list + toggle UI
    ├── vision.js           # camera-frame clutter analysis → pivot hint
    ├── drive.js            # held-input → motor-command translator
    ├── logger.js           # session logger (events → server SQLite)
    ├── snapshot.js         # camera-frame → JPEG capture for events
    ├── picker.js           # picker page entry
    └── control.js          # control page entry — wires everything together
```

No build step, no dependencies, no node_modules. ES modules served as plain
files via any local HTTP server. Open the source to see what they do.

---

## How to run

You can't open `index.html` directly with `file://` — modern browsers block
WebSocket connections from `file://` origins. You need a tiny local web
server. Easiest:

```bash
cd web
./serve.sh
```

That starts Python's built-in HTTP server on port 8000 and opens
`http://localhost:8000/` in your default browser. To use a different port:

```bash
./serve.sh 8080
```

Equivalent npm shortcut (same script, just convenient if you already
`npm install`ed for tests):

```bash
npm start
```

### Manual fallback

If you don't have `python3` or want full control:

```bash
# Node.js
npx serve -l 8000 .

# or pick whatever HTTP server you prefer; just serve the web/ directory.
```

Then open `http://localhost:8000/`.

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

### Record & reverse-replay ("come back")

The control page can record your moves and play them back **in reverse**,
which (in theory) walks the rover back to its starting position. There's
no real odometry — it's pure timing-based replay — so it's accurate when
the rover doesn't slip and the floor is flat.

How to use:

1. **Tap the red record button** in the side controls. The button pulses
   while recording and a small badge shows the move count.
2. Drive the rover normally with the d-pad (or arrow keys). Each time
   you press-and-hold a direction button, a `{action, duration}` entry
   gets stored. Sub-100 ms taps are ignored.
3. **Tap the record button again to stop**.
4. **Tap the reverse-arrow (↺) button** to replay. The page walks the
   recorded list **backwards**, **inverting each direction**:
   - `forward` → `backward`
   - `backward` → `forward`
   - `left` → `right`
   - `right` → `left`
5. While replaying, manually pressing any d-pad button **cancels the
   replay** so you can take over. You can also tap the replay button
   itself to abort.
6. **Long-press** the record button (≈700 ms) to clear the recording
   without starting a new one.

During replay the page pumps a keepalive command every 700 ms so the
rover's 1500 ms motor-deadman doesn't stop us mid-move.

### Autonomous mode (obstacle avoidance)

Tap the lightning-bolt icon next to the reboot button to start. The page
takes over the WS (telemetry polling pauses) and runs a state machine
that drives forward and dynamically scales PWM with the free distance
ahead — close to `STOP_CM` (~22 cm) it uses `PWM_MIN` (180), at/over
`FAR_CM` (~70 cm) it uses `PWM_MAX` (255), linear in between.

When an obstacle comes inside `STOP_CM` the rover stops, then pivots in
short bursts (180 ms) re-reading the GP2Y0A21 between each burst until
clearance (`CLEAR_CM` ≈ 45 cm) is found. The initial pivot side
**alternates each encounter** so it can't lock into spinning the same
way. If a single side fails to clear within 3 s, it backs up briefly,
flips to the other side, and tries again. All pivoting uses a fixed
`PIVOT_PWM` (180) for predictable angular increments.

Stopping conditions:

- Tap the auto button again, or any d-pad button, or press an arrow key.
- Disconnect / WS close.
- 5-minute hard cap (`HARD_CAP_MS`) — battery + walked-away safety.

When auto exits the user's pre-auto slider PWM is restored to the rover
so the next manual move uses the speed the user had selected.

Tunables live at the top of `js/autonomous.js`:
`STOP_CM`, `CLEAR_CM`, `FAR_CM`, `PWM_MIN`, `PWM_MAX`, `PIVOT_PWM`,
`PIVOT_BURST_MS`, `MAX_PIVOT_MS`, `BACKUP_MS`, `HARD_CAP_MS`, `TICK_MS`.

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
