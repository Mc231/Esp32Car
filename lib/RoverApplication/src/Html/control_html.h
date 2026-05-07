// control_html.h
#ifndef CONTROL_HTML_H
#define CONTROL_HTML_H

#include <Arduino.h>

const char control_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover, user-scalable=no">
<meta name="theme-color" content="#0b1020">
<title>Rover Control</title>
<style>
  :root {
    --bg: #0b1020;
    --bg-elev: #131a30;
    --bg-card: #1a2240;
    --border: #2a345a;
    --text: #e7ebf5;
    --text-dim: #8a93b3;
    --accent: #5b8cff;
    --accent-2: #8a5bff;
    --success: #3ddc84;
    --warning: #ffb454;
    --error: #ff6b6b;
    --shadow: 0 10px 30px rgba(0,0,0,.45);
  }
  @media (prefers-color-scheme: light) {
    :root {
      --bg: #f3f5fb;
      --bg-elev: #ffffff;
      --bg-card: #ffffff;
      --border: #e2e6f0;
      --text: #161a2b;
      --text-dim: #5e6680;
      --shadow: 0 8px 24px rgba(20,30,60,.10);
    }
  }
  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
  html, body { height: 100%; }
  body {
    margin: 0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, sans-serif;
    color: var(--text);
    background: radial-gradient(1200px 600px at 50% -10%, rgba(91,140,255,.15), transparent), var(--bg);
    -webkit-font-smoothing: antialiased;
    overflow: hidden;
    padding-top: env(safe-area-inset-top);
    padding-bottom: env(safe-area-inset-bottom);
  }

  .app {
    display: grid;
    grid-template-rows: auto 1fr auto;
    height: 100vh;
    max-height: 100dvh;
  }

  /* ==== Header ==== */
  header.bar {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--border);
    background: var(--bg-elev);
    z-index: 10;
  }
  .brand {
    display: flex; align-items: center; gap: 10px;
    font-weight: 650; letter-spacing: -.01em;
  }
  .logo {
    width: 32px; height: 32px;
    border-radius: 9px;
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    display: inline-flex; align-items: center; justify-content: center;
    box-shadow: var(--shadow);
  }
  .logo svg { width: 18px; height: 18px; color: #fff; }

  .stats {
    display: flex; gap: 6px; flex: 1; justify-content: flex-end; align-items: center;
    flex-wrap: wrap;
  }
  .pill {
    display: inline-flex; align-items: center; gap: 6px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    color: var(--text-dim);
    padding: 5px 9px;
    border-radius: 999px;
    font-size: 12px;
    line-height: 1;
  }
  .pill b { color: var(--text); font-weight: 600; }
  .dot { width: 6px; height: 6px; border-radius: 50%; background: var(--text-dim); display: inline-block; }
  .dot.ok { background: var(--success); box-shadow: 0 0 6px var(--success); }
  .dot.warn { background: var(--warning); }
  .dot.err { background: var(--error); }

  .icon-btn {
    background: var(--bg-card);
    border: 1px solid var(--border);
    color: var(--text);
    border-radius: 10px;
    width: 36px; height: 36px;
    display: inline-flex; align-items: center; justify-content: center;
    cursor: pointer;
  }
  .icon-btn svg { width: 17px; height: 17px; }
  .icon-btn:active { transform: scale(.94); }

  /* ==== Stage (camera + overlays) ==== */
  .stage {
    position: relative;
    overflow: hidden;
    display: flex; align-items: center; justify-content: center;
    background: #000;
  }
  #cameraStream {
    width: 100%;
    height: 100%;
    border: none;
    display: block;
  }
  .stage-empty {
    color: var(--text-dim);
    text-align: center;
    padding: 24px;
  }
  .stage-empty .spinner {
    width: 28px; height: 28px;
    border: 3px solid rgba(255,255,255,.15);
    border-top-color: var(--accent);
    border-radius: 50%;
    animation: spin .8s linear infinite;
    margin-bottom: 10px;
  }
  @keyframes spin { to { transform: rotate(360deg); } }

  .overlay-stat {
    position: absolute;
    background: rgba(11,16,32,.65);
    backdrop-filter: blur(10px);
    -webkit-backdrop-filter: blur(10px);
    color: #fff;
    border-radius: 10px;
    padding: 6px 10px;
    font-size: 12px;
    border: 1px solid rgba(255,255,255,.08);
    pointer-events: none;
  }
  .overlay-stat b { font-weight: 600; }
  .overlay-stat.tl { top: 12px; left: 12px; }
  .overlay-stat.tr { top: 12px; right: 12px; }
  .overlay-stat.bl { bottom: 12px; left: 12px; }
  .overlay-stat.br { bottom: 12px; right: 12px; }

  /* ==== Controls ==== */
  .controls {
    display: grid;
    grid-template-columns: auto 1fr auto;
    gap: 16px;
    padding: 14px 16px 18px;
    border-top: 1px solid var(--border);
    background: var(--bg-elev);
    align-items: center;
  }

  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 56px);
    grid-template-rows: repeat(3, 56px);
    gap: 6px;
    user-select: none;
  }
  .dpad button {
    background: var(--bg-card);
    border: 1px solid var(--border);
    color: var(--text);
    border-radius: 12px;
    display: inline-flex; align-items: center; justify-content: center;
    cursor: pointer;
    transition: background .1s, transform .08s;
  }
  .dpad button:active, .dpad button.active {
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    color: #fff;
    transform: scale(.96);
    border-color: transparent;
  }
  .dpad svg { width: 22px; height: 22px; }
  .dpad .up    { grid-column: 2; grid-row: 1; }
  .dpad .left  { grid-column: 1; grid-row: 2; }
  .dpad .stop  { grid-column: 2; grid-row: 2; background: transparent; border-style: dashed; color: var(--text-dim); }
  .dpad .stop:active, .dpad .stop.active {
    background: var(--bg-card);
    border-style: solid;
    color: var(--error);
    border-color: var(--error);
  }
  .dpad .right { grid-column: 3; grid-row: 2; }
  .dpad .down  { grid-column: 2; grid-row: 3; }

  .mid {
    display: flex;
    flex-direction: column;
    gap: 8px;
    min-width: 0;
  }
  .speed-row {
    display: flex; align-items: center; gap: 12px;
  }
  .speed-row label { font-size: 12px; color: var(--text-dim); white-space: nowrap; }
  .speed-row .value {
    font-variant-numeric: tabular-nums;
    color: var(--text);
    font-weight: 600;
    min-width: 36px;
    text-align: right;
  }
  input[type=range] {
    -webkit-appearance: none; appearance: none;
    flex: 1;
    height: 6px;
    background: var(--bg-card);
    border-radius: 999px;
    outline: none;
    border: 1px solid var(--border);
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none; appearance: none;
    width: 22px; height: 22px;
    border-radius: 50%;
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    cursor: pointer;
    box-shadow: 0 2px 6px rgba(0,0,0,.4);
    border: 2px solid var(--bg-elev);
  }
  input[type=range]::-moz-range-thumb {
    width: 22px; height: 22px;
    border-radius: 50%;
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    cursor: pointer;
    border: 2px solid var(--bg-elev);
  }

  .telemetry {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
    font-size: 12px;
  }
  .tel {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 6px 8px;
    display: flex; justify-content: space-between; gap: 8px;
  }
  .tel .k { color: var(--text-dim); }
  .tel .v { font-variant-numeric: tabular-nums; font-weight: 600; }

  .side-actions {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  /* ==== Drawer ==== */
  .scrim {
    position: fixed; inset: 0;
    background: rgba(0,0,0,.55);
    opacity: 0; pointer-events: none;
    transition: opacity .2s;
    z-index: 49;
  }
  .scrim.open { opacity: 1; pointer-events: auto; }
  .drawer {
    position: fixed;
    top: 0; right: 0; bottom: 0;
    width: min(380px, 92vw);
    background: var(--bg-elev);
    border-left: 1px solid var(--border);
    transform: translateX(100%);
    transition: transform .25s ease;
    box-shadow: var(--shadow);
    z-index: 50;
    display: flex; flex-direction: column;
    padding-top: env(safe-area-inset-top);
    padding-bottom: env(safe-area-inset-bottom);
  }
  .drawer.open { transform: translateX(0); }
  .drawer header {
    display: flex; align-items: center; justify-content: space-between;
    padding: 14px 16px;
    border-bottom: 1px solid var(--border);
  }
  .drawer h2 { margin: 0; font-size: 15px; font-weight: 650; }
  .drawer .body { padding: 14px 16px; overflow-y: auto; }
  .field { margin-bottom: 14px; }
  .field label { display: block; color: var(--text-dim); font-size: 12px; margin-bottom: 6px; text-transform: uppercase; letter-spacing: .06em; }
  .field input, .field select {
    width: 100%;
    padding: 10px 12px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 10px;
    color: var(--text);
    font-size: 14px;
    outline: none;
  }
  .field input:focus, .field select:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(91,140,255,.18); }

  .section-label {
    font-size: 11px; text-transform: uppercase; letter-spacing: .08em;
    color: var(--text-dim); font-weight: 600;
    margin: 16px 0 8px;
  }
  .section-label:first-child { margin-top: 0; }
  .info-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
    margin-bottom: 4px;
  }
  .info {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 8px 10px;
    display: flex; flex-direction: column; gap: 2px;
    min-width: 0;
  }
  .info .k { font-size: 10px; text-transform: uppercase; letter-spacing: .06em; color: var(--text-dim); }
  .info .v { font-size: 13px; font-weight: 600; font-variant-numeric: tabular-nums; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }

  button.danger, button.ghost {
    width: 100%;
    padding: 12px;
    border-radius: 10px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
    margin-top: 8px;
  }
  button.danger {
    background: rgba(255,107,107,.12);
    color: var(--error);
    border: 1px solid rgba(255,107,107,.35);
  }
  button.danger:active { background: rgba(255,107,107,.2); }
  button.ghost {
    background: transparent;
    color: var(--text);
    border: 1px solid var(--border);
  }

  .toast {
    position: fixed;
    left: 50%; bottom: 24px;
    transform: translateX(-50%) translateY(20px);
    background: var(--bg-card);
    border: 1px solid var(--border);
    color: var(--text);
    padding: 10px 14px;
    border-radius: 10px;
    box-shadow: var(--shadow);
    font-size: 13px;
    opacity: 0;
    transition: opacity .2s, transform .2s;
    z-index: 60;
    pointer-events: none;
    max-width: 90vw;
  }
  .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
  .toast.error { border-color: var(--error); color: var(--error); }
  .toast.success { border-color: var(--success); color: var(--success); }

  /* ==== Mobile portrait: stack ==== */
  @media (max-width: 720px) {
    .controls { grid-template-columns: 1fr; gap: 14px; }
    .side-actions { flex-direction: row; justify-content: flex-end; }
    .dpad { justify-self: center; }
    .stats .pill { display: none; }
    .stats .pill.always { display: inline-flex; }
  }
</style>
</head>
<body>
<div class="app">
  <header class="bar">
    <div class="brand">
      <div class="logo">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
          <rect x="3" y="8" width="18" height="10" rx="2"/>
          <circle cx="7.5" cy="18" r="1.6"/>
          <circle cx="16.5" cy="18" r="1.6"/>
          <path d="M7 8V5h10v3"/>
        </svg>
      </div>
      <span>Rover</span>
    </div>
    <div class="stats">
      <span class="pill always" id="connPill"><span class="dot" id="connDot"></span><b id="connText">…</b></span>
      <span class="pill"><span>IP</span><b id="ipText">—</b></span>
      <span class="pill"><span>Up</span><b id="upText">—</b></span>
      <span class="pill"><span>Heap</span><b id="heapText">—</b></span>
    </div>
    <button class="icon-btn" id="settingsBtn" title="Settings" aria-label="Settings">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="3"/>
        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/>
      </svg>
    </button>
  </header>

  <section class="stage">
    <iframe id="cameraStream" allow="autoplay" referrerpolicy="no-referrer"></iframe>
    <div class="overlay-stat tl" id="distStat"><b>—</b> cm</div>
    <div class="overlay-stat tr" id="speedStat">PWM <b>0</b></div>
    <div class="overlay-stat bl" id="motorStatL">L: —</div>
    <div class="overlay-stat br" id="motorStatR">R: —</div>
  </section>

  <section class="controls">
    <div class="dpad" id="dpad">
      <button data-act="forward" class="up"   aria-label="Forward">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="18 15 12 9 6 15"/></svg>
      </button>
      <button data-act="left" class="left" aria-label="Turn left">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"/></svg>
      </button>
      <button data-act="stop" class="stop" aria-label="Stop">
        <svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="6" width="12" height="12" rx="2"/></svg>
      </button>
      <button data-act="right" class="right" aria-label="Turn right">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
      </button>
      <button data-act="backward" class="down" aria-label="Backward">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="6 9 12 15 18 9"/></svg>
      </button>
    </div>

    <div class="mid">
      <div class="speed-row">
        <label for="speed">Speed</label>
        <input type="range" id="speed" min="0" max="255" value="160">
        <span class="value" id="speedVal">160</span>
      </div>
      <div class="telemetry">
        <div class="tel"><span class="k">Distance</span><span class="v" id="telDist">—</span></div>
        <div class="tel"><span class="k">Uptime</span><span class="v" id="telUp">—</span></div>
        <div class="tel"><span class="k">Free heap</span><span class="v" id="telHeap">—</span></div>
        <div class="tel"><span class="k">IP</span><span class="v" id="telIp">—</span></div>
      </div>
    </div>

    <div class="side-actions">
      <button class="icon-btn" id="cameraBtn" title="Camera resolution" aria-label="Camera resolution">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/>
        </svg>
      </button>
      <button class="icon-btn" id="rebootBtn" title="Reboot" aria-label="Reboot">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <polyline points="23 4 23 10 17 10"/><path d="M20.49 15A9 9 0 1 1 18 5.36"/>
        </svg>
      </button>
    </div>
  </section>
</div>

<div class="scrim" id="scrim"></div>
<aside class="drawer" id="drawer">
  <header>
    <h2>Settings</h2>
    <button class="icon-btn" id="closeDrawer" aria-label="Close">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
    </button>
  </header>
  <div class="body">
    <div class="section-label">Camera</div>
    <div class="field">
      <label for="resolution">Resolution</label>
      <select id="resolution">
        <option value="0">96 × 96</option>
        <option value="1">160 × 120</option>
        <option value="2">176 × 144</option>
        <option value="3">240 × 176</option>
        <option value="4">240 × 240</option>
        <option value="5">320 × 240</option>
        <option value="6">400 × 296</option>
        <option value="7" selected>480 × 320</option>
        <option value="8">640 × 480</option>
        <option value="9">800 × 600</option>
        <option value="10">1024 × 768</option>
        <option value="11">1280 × 720</option>
        <option value="12">1280 × 1024</option>
        <option value="13">1600 × 1200</option>
      </select>
    </div>
    <div class="field">
      <label for="interval">Telemetry Interval (ms)</label>
      <input type="number" id="interval" min="200" step="100" value="1000">
    </div>

    <div class="section-label">Network</div>
    <div class="info-grid">
      <div class="info"><span class="k">SSID</span><span class="v" id="infoSsid">—</span></div>
      <div class="info"><span class="k">IP</span><span class="v" id="infoIp">—</span></div>
      <div class="info"><span class="k">Uptime</span><span class="v" id="infoUp">—</span></div>
      <div class="info"><span class="k">Free heap</span><span class="v" id="infoHeap">—</span></div>
    </div>

    <div class="section-label">Hardware Pins</div>
    <div class="info-grid" id="pinGrid">
      <div class="info"><span class="k">…</span><span class="v">loading</span></div>
    </div>

    <div class="section-label">Actions</div>
    <button class="ghost" id="refreshInfoBtn">Refresh Info</button>
    <button class="ghost" onclick="window.open('/logs','_blank')">Live Logs</button>
    <button class="ghost" onclick="window.open('/ota','_blank')">Update Firmware (OTA)</button>
    <button class="ghost" id="forgetBtn">Forget Wi-Fi</button>
    <button class="danger" id="rebootBtn2">Reboot Rover</button>
  </div>
</aside>

<div class="toast" id="toast"></div>

<script>
(() => {
  const $ = id => document.getElementById(id);

  // ===== State =====
  const state = {
    ip: null,
    speed: 160,
    pollInterval: 1000,
    pollTimer: null,
    online: false,
  };

  // ===== Toast =====
  let toastTimer;
  function toast(msg, kind = '') {
    const el = $('toast');
    el.textContent = msg;
    el.className = 'toast show ' + kind;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => el.className = 'toast', 2200);
  }

  // ===== HTTP helpers =====
  function api(path, opts = {}) {
    return fetch(path, opts).then(r => {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r;
    });
  }

  // ===== Connect status =====
  function setOnline(ok) {
    state.online = ok;
    $('connDot').className = 'dot ' + (ok ? 'ok' : 'err');
    $('connText').textContent = ok ? 'Online' : 'Offline';
  }

  // ===== Camera =====
  function setStreamUrl(ip) {
    if (!ip) return;
    $('cameraStream').src = 'http://' + ip + ':81/stream';
  }

  // ===== Settings =====
  function openDrawer()  { $('drawer').classList.add('open'); $('scrim').classList.add('open'); }
  function closeDrawer() { $('drawer').classList.remove('open'); $('scrim').classList.remove('open'); }
  $('settingsBtn').addEventListener('click', openDrawer);
  $('cameraBtn').addEventListener('click', openDrawer);
  $('closeDrawer').addEventListener('click', closeDrawer);
  $('scrim').addEventListener('click', closeDrawer);

  $('resolution').addEventListener('change', e => {
    api('/camera', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ frame_size: parseInt(e.target.value) }),
    })
      .then(() => toast('Camera updated', 'success'))
      .catch(() => toast('Camera update failed', 'error'));
  });

  $('interval').addEventListener('change', e => {
    const v = Math.max(200, parseInt(e.target.value) || 1000);
    state.pollInterval = v;
    e.target.value = v;
    restartPolling();
    toast('Interval set to ' + v + ' ms');
  });

  // ===== Telemetry =====
  function fmtUptime(s) {
    s = Math.max(0, parseInt(s) || 0);
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60;
    if (h) return h + 'h ' + m + 'm';
    if (m) return m + 'm ' + ss + 's';
    return ss + 's';
  }
  function fmtBytes(n) {
    n = parseInt(n) || 0;
    if (n >= 1024 * 1024) return (n / 1048576).toFixed(2) + ' MB';
    if (n >= 1024) return (n / 1024).toFixed(1) + ' KB';
    return n + ' B';
  }
  const ACTION_LABEL = { 0: 'F', 1: 'B', 2: 'S' };

  function poll() {
    api('/status').then(r => r.json()).then(d => {
      setOnline(true);
      if (d.system) {
        const up = fmtUptime(d.system.up_time);
        const heap = fmtBytes(d.system.free_heap);
        $('upText').textContent = up;
        $('telUp').textContent  = up;
        $('infoUp').textContent = up;
        $('heapText').textContent = heap;
        $('telHeap').textContent  = heap;
        $('infoHeap').textContent = heap;
      }
      if (d.distance && d.distance.last_distance != null) {
        const cm = parseFloat(d.distance.last_distance).toFixed(1);
        $('telDist').textContent = cm + ' cm';
        $('distStat').innerHTML = '<b>' + cm + '</b> cm';
      } else {
        $('telDist').textContent = '—';
        $('distStat').innerHTML  = '<b>—</b> cm';
      }
      if (d.motor) {
        $('motorStatL').textContent = 'L: ' + (ACTION_LABEL[d.motor.lma] || '?') + ' ' + (d.motor.lmpwm ?? 0);
        $('motorStatR').textContent = 'R: ' + (ACTION_LABEL[d.motor.rma] || '?') + ' ' + (d.motor.rmpwm ?? 0);
      }
    }).catch(() => setOnline(false));
  }
  function restartPolling() {
    if (state.pollTimer) clearInterval(state.pollTimer);
    state.pollTimer = setInterval(poll, state.pollInterval);
  }

  // ===== Wi-Fi + hardware info =====
  function loadWifi() {
    return api('/wifi').then(r => r.json()).then(d => {
      state.ip = d.ip;
      $('ipText').textContent = d.ip || '—';
      $('telIp').textContent  = d.ip || '—';
      $('infoIp').textContent = d.ip || '—';
      $('infoSsid').textContent = d.ssid || '—';
      setStreamUrl(d.ip || location.hostname);
    });
  }
  function loadConfig() {
    return api('/config').then(r => r.json()).then(d => {
      const grid = $('pinGrid');
      grid.innerHTML = '';
      const rows = [
        ['Left motor',  `${d.leftMotorPin1} / ${d.leftMotorPin2}`],
        ['Left PWM',    d.leftMotorPwm],
        ['Right motor', `${d.rightMotorPin1} / ${d.rightMotorPin2}`],
        ['Right PWM',   d.rightMotorPwm],
        ['Distance sensor', d.distanceSensorPin],
      ];
      rows.forEach(([k, v]) => {
        const el = document.createElement('div');
        el.className = 'info';
        el.innerHTML = `<span class="k"></span><span class="v"></span>`;
        el.querySelector('.k').textContent = k;
        el.querySelector('.v').textContent = v ?? '—';
        grid.appendChild(el);
      });
    });
  }
  function loadAllInfo() {
    Promise.all([loadWifi(), loadConfig()])
      .catch(() => toast('Could not load info', 'error'));
  }
  loadWifi().catch(() => { setStreamUrl(location.hostname); toast('Could not read Wi-Fi info', 'error'); });
  loadConfig().catch(() => {});
  $('refreshInfoBtn').addEventListener('click', () => { loadAllInfo(); toast('Info refreshed'); });

  // ===== Speed =====
  function applySpeed(v, motor /* 0=left, 1=right, 2=both */) {
    const motors = motor === 2 ? [0, 1] : [motor];
    motors.forEach(m => {
      api('/motorPWM', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ motor: m, pwm: String(v) }),
      }).catch(() => {});
    });
  }
  $('speed').addEventListener('input', e => {
    state.speed = parseInt(e.target.value);
    $('speedVal').textContent = state.speed;
    $('speedStat').innerHTML = 'PWM <b>' + state.speed + '</b>';
  });
  $('speed').addEventListener('change', e => applySpeed(state.speed, 2));

  // ===== Motor control =====
  // action: 0 forward, 1 backward, 2 stop. motor: 0 left, 1 right, 2 both.
  function motor(action, m) {
    return api('/motor', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ action: action, motor: m }),
    }).catch(() => {});
  }

  function dispatch(act) {
    if (act === 'forward')  return motor(0, 2);
    if (act === 'backward') return motor(1, 2);
    if (act === 'stop')     return motor(2, 2);
    if (act === 'left')     { motor(2, 0); return motor(0, 1); }   // turn left: stop left wheel, drive right wheel forward
    if (act === 'right')    { motor(2, 1); return motor(0, 0); }   // turn right: stop right wheel, drive left wheel forward
  }

  // ===== D-pad bindings (touch + mouse) =====
  const buttons = document.querySelectorAll('.dpad button');
  let heldAction = null;        // current held button's action
  let keepaliveTimer = null;    // re-sends the command to feed the rover's deadman

  function startKeepalive(act) {
    heldAction = act;
    clearInterval(keepaliveTimer);
    keepaliveTimer = setInterval(() => { if (heldAction) dispatch(heldAction); }, 700);
  }
  function stopKeepalive() {
    heldAction = null;
    clearInterval(keepaliveTimer);
    keepaliveTimer = null;
  }

  const releaseAll = () => {
    buttons.forEach(b => b.classList.remove('active'));
    stopKeepalive();
    motor(2, 2);
  };

  buttons.forEach(btn => {
    const act = btn.dataset.act;
    const press = e => {
      e.preventDefault();
      btn.classList.add('active');
      dispatch(act);
      if (act !== 'stop') startKeepalive(act);
    };
    const release = e => {
      e.preventDefault();
      btn.classList.remove('active');
      if (act !== 'stop') { stopKeepalive(); motor(2, 2); }
    };
    btn.addEventListener('pointerdown', press);
    btn.addEventListener('pointerup', release);
    btn.addEventListener('pointerleave', release);
    btn.addEventListener('pointercancel', release);
  });

  // ===== Keyboard =====
  const keyMap = { ArrowUp: 'forward', ArrowDown: 'backward', ArrowLeft: 'left', ArrowRight: 'right', ' ': 'stop' };
  const pressed = new Set();
  document.addEventListener('keydown', e => {
    const act = keyMap[e.key];
    if (!act || pressed.has(e.key)) return;
    pressed.add(e.key);
    e.preventDefault();
    document.querySelector('.dpad .' + ({forward:'up',backward:'down',left:'left',right:'right',stop:'stop'}[act]))?.classList.add('active');
    dispatch(act);
    if (act !== 'stop') startKeepalive(act);
  });
  document.addEventListener('keyup', e => {
    if (!keyMap[e.key]) return;
    pressed.delete(e.key);
    const act = keyMap[e.key];
    document.querySelector('.dpad .' + ({forward:'up',backward:'down',left:'left',right:'right',stop:'stop'}[act]))?.classList.remove('active');
    if (act !== 'stop') { stopKeepalive(); motor(2, 2); }
  });
  window.addEventListener('blur', releaseAll);

  // ===== Reboot / Forget =====
  function reboot() {
    if (!confirm('Reboot the rover?')) return;
    api('/reboot', { method: 'POST' })
      .then(() => { toast('Rebooting...', 'success'); setOnline(false); })
      .catch(() => toast('Reboot failed', 'error'));
  }
  $('rebootBtn').addEventListener('click', reboot);
  $('rebootBtn2').addEventListener('click', reboot);
  $('forgetBtn').addEventListener('click', () => {
    if (!confirm('Erase saved Wi-Fi credentials and reboot?')) return;
    api('/wifi/forget', { method: 'POST' })
      .then(() => toast('Wi-Fi cleared, rebooting...', 'success'))
      .catch(() => toast('Failed', 'error'));
  });

  // Initial paint
  $('speedVal').textContent = state.speed;
  $('speedStat').innerHTML = 'PWM <b>' + state.speed + '</b>';
  poll();
  restartPolling();
})();
</script>
</body>
</html>
)rawliteral";

#endif // CONTROL_HTML_H
