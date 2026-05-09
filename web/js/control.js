// Control page entry — wires WS transport, telemetry polling, recording,
// and input bindings together.

import { createConnection } from './ws.js';
import { toast } from './toast.js';
import { createRecorder } from './recording.js';
import { createAutonomous } from './autonomous.js';
import { bindControls } from './controls-input.js';
import { fetchTransports, toggleTransport, renderTransports, labelFor } from './transports.js';
import { createVisionAnalyzer } from './vision.js';
import { computeMotorCommands } from './drive.js';
import { createLogger } from './logger.js';
import { createSnapshotter } from './snapshot.js';

const $ = id => document.getElementById(id);

// ---------- Resolve rover from URL ----------
const params = new URLSearchParams(location.search);
const host = params.get('host');
const name = params.get('name') || host || 'Rover';
if (!host) {
  document.body.innerHTML =
    '<div style="padding:48px;text-align:center;color:var(--text-dim);font-family:sans-serif">' +
    'No rover specified. <a href="index.html" style="color:var(--accent)">Pick one</a>.</div>';
  throw new Error('no host');
}
$('roverName').textContent = name;
document.title = `${name} — Rover`;

// Camera: ESP-Camera streams MJPEG on port 81.
$('cameraStream').src = `http://${host}:81/stream`;
$('cameraStream').onerror = () => {
  $('cameraStream').hidden = true;
  $('camFallback').hidden = false;
};

$('backBtn').addEventListener('click', () => location.href = 'index.html');

// ---------- State ----------
const state = {
  online: false,
  speed: 160,
  pollTimer: null,
  pollInFlight: false,
};

function setOnline(ok) {
  state.online = ok;
  $('connDot').className = 'dot ' + (ok ? 'ok' : 'err');
  $('connText').textContent = ok ? 'Online' : 'Offline';
}

// ---------- Telemetry logger ----------
// Captures every interesting event (manual inputs, autonomous decisions,
// vision samples, motor commands) into the local server's SQLite store
// for offline analysis. Fails open if the server isn't reachable.
const logger = createLogger({ host, name });
logger.start();
window.addEventListener('beforeunload', () => logger.flushBeacon());
window.addEventListener('pagehide',     () => logger.flushBeacon());

// ---------- WebSocket connection ----------
const conn = createConnection({
  host,
  onOpen: () => {
    setOnline(true);
    startPolling();
    loadStaticInfo();
    logger.event('ws', { state: 'open' });
  },
  onClose: () => {
    setOnline(false);
    stopPolling();
    logger.event('ws', { state: 'close' });
  },
});

function send(payload, opts) { return conn.send(payload, opts); }

// ---------- Telemetry ----------
const fmtUptime = s => {
  s = Math.max(0, parseInt(s) || 0);
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60;
  if (h) return `${h}h ${m}m`;
  if (m) return `${m}m ${ss}s`;
  return `${ss}s`;
};
const fmtBytes = n => {
  n = parseInt(n) || 0;
  if (n >= 1024 * 1024) return (n / 1048576).toFixed(2) + ' MB';
  if (n >= 1024) return (n / 1024).toFixed(1) + ' KB';
  return n + ' B';
};
const ACTION_LABEL = { 0: 'F', 1: 'B', 2: 'S' };

async function pollOnce() {
  if (state.pollInFlight) return;
  state.pollInFlight = true;
  try {
    const [sysMsg, distMsg, motMsg] = await Promise.all([
      send({ command: 'system' }),
      send({ command: 'distance' }),
      send({ command: 'motor_state' }),
    ]);

    const sys = sysMsg?.response || {};
    const up = fmtUptime(sys.up_time);
    const heap = fmtBytes(sys.free_heap);
    $('upText').textContent = up;
    $('telUp').textContent  = up;
    $('heapText').textContent = heap;
    $('telHeap').textContent  = heap;

    const dist = distMsg?.response || {};
    if (dist.last_distance != null && dist.last_distance >= 0) {
      const cm = parseFloat(dist.last_distance).toFixed(1);
      $('telDist').textContent = `${cm} cm`;
      $('distStat').innerHTML = `<b>${cm}</b> cm`;
    } else {
      $('telDist').textContent = '—';
      $('distStat').innerHTML  = '<b>—</b> cm';
    }

    const mot = motMsg?.response || {};
    $('motorStatL').textContent = `L: ${ACTION_LABEL[mot.lma] || '?'} ${mot.lmpwm ?? 0}`;
    $('motorStatR').textContent = `R: ${ACTION_LABEL[mot.rma] || '?'} ${mot.rmpwm ?? 0}`;
  } catch (e) {
    // Connection died — onClose will handle reconnect.
  } finally {
    state.pollInFlight = false;
  }
}

function startPolling() {
  stopPolling();
  if (auto?.isActive()) return;   // autonomous loop owns the WS while active
  pollOnce();
  state.pollTimer = setInterval(pollOnce, 1000);
}
function stopPolling() {
  if (state.pollTimer) clearInterval(state.pollTimer);
  state.pollTimer = null;
}

async function loadStaticInfo() {
  try {
    const wifi = await send({ command: 'wifi' });
    $('telIp').textContent = wifi?.response?.ip || host;
  } catch {}
}

// ---------- Motor commands ----------
// action: 0 = forward, 1 = backward, 2 = stop. motor: 0 = left, 1 = right, 2 = both.
function motor(action, m) {
  return send({ command: 'set_motor', action, motor: m }).catch(() => {});
}

function applySpeed(v, m = 2) {
  // PWM must be a number (server uses j["pwm"].get<int>()). Sanitise hard
  // so a bad slider value can't ever send NaN or out-of-range.
  const pwm = Math.max(0, Math.min(255, parseInt(v, 10) || 0));
  const motors = m === 2 ? [0, 1] : [m];
  motors.forEach(mm => send({ command: 'set_motor_pwm', motor: mm, pwm }).catch(() => {}));
}

// Live-input state machine: tracks which d-pad/keyboard inputs are
// currently held so combinations (forward + left = arc, not stop)
// produce a single combined motor command instead of fighting each
// other.
const heldInputs = new Set();
const lastSent   = { la: null, ra: null, lp: null, rp: null };

function applyHeldInputs() {
  const cmd = computeMotorCommands(heldInputs, state.speed);
  console.log(
    `[drive] held=${[...heldInputs].join('+') || 'none'} ` +
    `speed=${state.speed} ` +
    `→ L:a=${cmd.leftAction}/p=${cmd.leftPwm} ` +
    `R:a=${cmd.rightAction}/p=${cmd.rightPwm}`
  );
  logger.event('input', {
    held:  [...heldInputs],
    speed: state.speed,
    cmd,
  });
  // Only send what changed — keeps the WS FIFO small and avoids
  // re-issuing identical PWM commands at keepalive cadence.
  if (cmd.leftPwm  !== lastSent.lp) {
    send({ command: 'set_motor_pwm', motor: 0, pwm: cmd.leftPwm  }).catch(() => {});
    lastSent.lp = cmd.leftPwm;
  }
  if (cmd.rightPwm !== lastSent.rp) {
    send({ command: 'set_motor_pwm', motor: 1, pwm: cmd.rightPwm }).catch(() => {});
    lastSent.rp = cmd.rightPwm;
  }
  if (cmd.leftAction  !== lastSent.la) {
    send({ command: 'set_motor', motor: 0, action: cmd.leftAction  }).catch(() => {});
    lastSent.la = cmd.leftAction;
  }
  if (cmd.rightAction !== lastSent.ra) {
    send({ command: 'set_motor', motor: 1, action: cmd.rightAction }).catch(() => {});
    lastSent.ra = cmd.rightAction;
  }
}

// Two-arg form (kind = 'press' | 'release') comes from the d-pad /
// keyboard input layer. Single-arg form is used by the recorder and
// the autonomous loop and falls back to the original tank-style
// commands so existing recordings replay identically.
function dispatch(act, kind) {
  if (kind === 'refresh') {
    // Keepalive: clear the dedup cache and re-issue current commands
    // so the firmware deadman doesn't auto-stop motors while a key is
    // held without state changes.
    lastSent.la = lastSent.ra = lastSent.lp = lastSent.rp = null;
    applyHeldInputs();
    return;
  }
  if (kind === 'press' || kind === 'release') {
    if (kind === 'press') {
      if (act === 'stop') heldInputs.clear();
      else heldInputs.add(act);
    } else {
      heldInputs.delete(act);
    }
    applyHeldInputs();
    return;
  }
  // Legacy single-arg path — recorder playback, autonomous mode, etc.
  // Route through the same drive logic the live state machine uses so
  // PWM and action are sent together. Without this, replay only sends
  // set_motor (action) and motors stay at whatever PWM they had after
  // the last live release — usually 0, so playback wouldn't move.
  const transient = new Set();
  if (act === 'forward'  || act === 'backward' ||
      act === 'left'     || act === 'right') {
    transient.add(act);
  }
  // 'stop' → empty set → STOP_BOTH
  const cmd = computeMotorCommands(transient, state.speed);
  send({ command: 'set_motor_pwm', motor: 0, pwm: cmd.leftPwm  }).catch(() => {});
  send({ command: 'set_motor_pwm', motor: 1, pwm: cmd.rightPwm }).catch(() => {});
  send({ command: 'set_motor', motor: 0, action: cmd.leftAction  }).catch(() => {});
  send({ command: 'set_motor', motor: 1, action: cmd.rightAction }).catch(() => {});
  // Keep the live cache in sync — next d-pad press will see the real
  // current state instead of stale values from before playback ran.
  lastSent.la = cmd.leftAction;
  lastSent.ra = cmd.rightAction;
  lastSent.lp = cmd.leftPwm;
  lastSent.rp = cmd.rightPwm;
}

// ---------- Recorder + replay ----------
const recorder = createRecorder({
  dispatch,
  motor,
  onStateChange: s => {
    const recBtn   = $('recordBtn');
    const replayBtn = $('replayBtn');
    const badge    = $('recBadge');
    recBtn.classList.toggle('recording', s.recording);
    recBtn.title = s.recording ? 'Stop recording' : 'Start recording';
    replayBtn.classList.toggle('replaying', s.replaying);
    replayBtn.disabled = s.recording || s.replaying || s.moveCount === 0;
    if (s.moveCount > 0) {
      badge.hidden = false;
      badge.textContent = String(s.moveCount);
    } else {
      badge.hidden = true;
    }
  },
});

$('recordBtn').addEventListener('click', () => {
  if (recorder.isRecording()) {
    recorder.stopRecording();
    toast(`Recorded ${recorder.moveCount()} move${recorder.moveCount() === 1 ? '' : 's'}`,
          recorder.moveCount() > 0 ? 'success' : '');
  } else {
    recorder.startRecording();
    toast('Recording moves…');
  }
});

// Long-press the record button to clear without starting a new recording.
let recHoldTimer = null;
$('recordBtn').addEventListener('pointerdown', () => {
  clearTimeout(recHoldTimer);
  recHoldTimer = setTimeout(() => {
    if (recorder.moveCount() > 0 && confirm('Clear recorded moves?')) {
      recorder.clearRecording();
      toast('Recording cleared');
    }
  }, 700);
});
$('recordBtn').addEventListener('pointerup',    () => clearTimeout(recHoldTimer));
$('recordBtn').addEventListener('pointerleave', () => clearTimeout(recHoldTimer));

$('replayBtn').addEventListener('click', async () => {
  if (recorder.isReplaying()) {
    recorder.abortReplay();
    toast('Replay cancelled');
    return;
  }
  toast(`Replaying ${recorder.moveCount()} moves in reverse…`);
  const completed = await recorder.startReplay();
  if (completed) toast('Replay finished', 'success');
});

// ---------- Vision (camera-based pivot hint) ----------
// logEvery: heartbeat log every Nth sample so you can see vision is
// alive even when the rover isn't pivoting. 4 = one log every ~2 s.
const vision = createVisionAnalyzer({
  imgEl: $('cameraStream'),
  logEvery: 4,
  onSample: thirds => logger.event('vision', thirds),
});
vision.start();

// ---------- Snapshotter (per-event camera frame for analysis) ----------
const snapshotter = createSnapshotter({ imgEl: $('cameraStream') });
async function recordSnapshot(type, payload) {
  const blob = await snapshotter.captureJpeg();
  return logger.snapshotEvent(type, payload, blob);
}

// ---------- Autonomous mode ----------
let userSpeedBeforeAuto = state.speed;
const auto = createAutonomous({
  send,
  dispatch,
  motor,
  applyPwm: pwm => applySpeed(pwm, 2),
  vision,
  logger,
  onSnapshot: recordSnapshot,
  onTelemetry: ({ distance, pwm }) => {
    if (distance != null) {
      const cm = parseFloat(distance).toFixed(1);
      $('telDist').textContent = `${cm} cm`;
      $('distStat').innerHTML  = `<b>${cm}</b> cm`;
    }
    // Reflect auto's PWM in the overlay/slider but DON'T touch state.speed —
    // we want to restore the user's slider value when auto exits.
    if (pwm != null) {
      $('speed').value = pwm;
      $('speedVal').textContent = pwm;
      $('speedStat').innerHTML = `PWM <b>${pwm}</b>`;
    }
  },
  onStateChange: s => {
    const btn = $('autoBtn');
    btn.classList.toggle('active', s.active);
    btn.title = s.active ? `Autonomous: ${s.phase}` : 'Autonomous mode';
    if (s.active) {
      userSpeedBeforeAuto = state.speed;
      stopPolling();          // suspend 1Hz telemetry while auto owns the WS
    } else {
      // Push the user's pre-auto PWM back so the next manual move uses it.
      applySpeed(userSpeedBeforeAuto, 2);
      $('speed').value = userSpeedBeforeAuto;
      $('speedVal').textContent = userSpeedBeforeAuto;
      $('speedStat').innerHTML = `PWM <b>${userSpeedBeforeAuto}</b>`;
      if (state.online) startPolling();
    }
  },
});

$('autoBtn').addEventListener('click', () => {
  if (!state.online) { toast('Rover offline'); return; }
  if (auto.isActive()) {
    auto.stop();
    toast('Autonomous stopped');
  } else {
    auto.start();
    toast('Autonomous started — drive in 5 min cap', 'success');
  }
});

// ---------- Input bindings ----------
bindControls({
  dispatchFn: dispatch,
  onPress: act => {
    // Any manual d-pad press cancels autonomous mode.
    if (auto.isActive()) {
      auto.stop();
      toast('Autonomous cancelled');
    }
    recorder.onPress(act);
  },
  onRelease: act => recorder.onRelease(act),
});

// ---------- Speed slider ----------
$('speed').addEventListener('input', e => {
  state.speed = parseInt(e.target.value, 10);
  $('speedVal').textContent = state.speed;
  $('speedStat').innerHTML = `PWM <b>${state.speed}</b>`;
});
$('speed').addEventListener('change', e => {
  // Invalidate the held-input cache so the next press recomputes PWM
  // against the new slider value instead of skipping it as "unchanged".
  lastSent.lp = lastSent.rp = null;
  applySpeed(state.speed, 2);
  // If the user is currently holding a direction, immediately re-apply
  // so the live arc/turn picks up the new speed without releasing keys.
  if (heldInputs.size) applyHeldInputs();
});

// ---------- Transports panel ----------
const transportsModal = $('transportsModal');
const transportsList  = $('transportsList');

async function refreshTransports() {
  try {
    const list = await fetchTransports(send);
    renderTransports(transportsList, list, {
      onToggle: async (name, enabled, cb) => {
        cb.disabled = true;
        try {
          await toggleTransport(send, name, enabled);
          toast(`${labelFor(name)} ${enabled ? 'enabled' : 'disabled'}`,
                enabled ? 'success' : '');
        } catch (e) {
          cb.checked = !enabled;   // revert
          toast(`Failed: ${e.message}`);
        } finally {
          cb.disabled = false;
        }
      },
    });
  } catch (e) {
    transportsList.innerHTML =
      `<div class="tr-empty">Failed to load: ${e.message}</div>`;
  }
}

function openTransports() {
  if (!state.online) { toast('Rover offline'); return; }
  transportsModal.hidden = false;
  refreshTransports();
}
function closeTransports() { transportsModal.hidden = true; }

$('transportsBtn').addEventListener('click', openTransports);
$('transportsCloseBtn').addEventListener('click', e => {
  e.stopPropagation();
  closeTransports();
});
// Backdrop click — only the backdrop element itself, not bubbled clicks
// from inside `.modal`. Use mousedown so a drag that ends inside the
// modal doesn't accidentally close.
transportsModal.addEventListener('mousedown', e => {
  if (e.target === transportsModal) closeTransports();
});
// Escape key — universal "dismiss this modal" expectation.
document.addEventListener('keydown', e => {
  if (e.key === 'Escape' && !transportsModal.hidden) closeTransports();
});

// ---------- Reboot ----------
$('rebootBtn').addEventListener('click', () => {
  if (!confirm('Reboot the rover?')) return;
  send({ command: 'reboot' }, { fireAndForget: true });
  toast('Rebooting…', 'success');
  setOnline(false);
});

// Initial paint.
$('speedVal').textContent = state.speed;
$('speedStat').innerHTML = `PWM <b>${state.speed}</b>`;
setOnline(false);
