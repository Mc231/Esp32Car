// Control page entry — wires WS transport, telemetry polling, recording,
// and input bindings together.

import { createConnection } from './ws.js';
import { toast } from './toast.js';
import { createRecorder } from './recording.js';
import { createAutonomous } from './autonomous.js';
import { bindControls } from './controls-input.js';

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

// ---------- WebSocket connection ----------
const conn = createConnection({
  host,
  onOpen: () => {
    setOnline(true);
    startPolling();
    loadStaticInfo();
  },
  onClose: () => {
    setOnline(false);
    stopPolling();
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

function dispatch(act) {
  if (act === 'forward')  return motor(0, 2);
  if (act === 'backward') return motor(1, 2);
  if (act === 'stop')     return motor(2, 2);
  if (act === 'left')     { motor(2, 0); return motor(0, 1); }
  if (act === 'right')    { motor(2, 1); return motor(0, 0); }
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

// ---------- Autonomous mode ----------
let userSpeedBeforeAuto = state.speed;
const auto = createAutonomous({
  send,
  dispatch,
  motor,
  applyPwm: pwm => applySpeed(pwm, 2),
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
$('speed').addEventListener('change', e => applySpeed(state.speed, 2));

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
