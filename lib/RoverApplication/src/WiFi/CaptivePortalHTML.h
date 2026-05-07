// CaptivePortalHTML.h
#ifndef CaptivePortalHTML_h
#define CaptivePortalHTML_h

#include <Arduino.h>

const char captivePortalHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0b1020">
<title>Rover Setup</title>
<style>
  :root {
    --bg: #0b1020;
    --bg-elev: #131a30;
    --bg-card: #1a2240;
    --border: #2a345a;
    --text: #e7ebf5;
    --text-dim: #8a93b3;
    --accent: #5b8cff;
    --accent-hover: #7aa1ff;
    --success: #3ddc84;
    --error: #ff6b6b;
    --shadow: 0 10px 30px rgba(0,0,0,.35);
  }
  @media (prefers-color-scheme: light) {
    :root {
      --bg: #f3f5fb;
      --bg-elev: #ffffff;
      --bg-card: #ffffff;
      --border: #e2e6f0;
      --text: #161a2b;
      --text-dim: #5e6680;
      --shadow: 0 8px 24px rgba(20,30,60,.08);
    }
  }
  * { box-sizing: border-box; }
  html, body { height: 100%; }
  body {
    margin: 0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, sans-serif;
    color: var(--text);
    background: radial-gradient(1200px 600px at 50% -10%, rgba(91,140,255,.15), transparent),
                var(--bg);
    -webkit-font-smoothing: antialiased;
    padding: env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left);
    display: flex;
    align-items: flex-start;
    justify-content: center;
  }
  main {
    width: 100%;
    max-width: 460px;
    padding: 32px 20px 40px;
  }
  header {
    text-align: center;
    margin-bottom: 22px;
  }
  .logo {
    width: 56px; height: 56px;
    border-radius: 16px;
    background: linear-gradient(135deg, var(--accent) 0%, #8a5bff 100%);
    display: inline-flex;
    align-items: center;
    justify-content: center;
    box-shadow: var(--shadow);
    margin-bottom: 12px;
  }
  .logo svg { width: 28px; height: 28px; color: white; }
  h1 { font-size: 22px; margin: 0 0 4px; font-weight: 650; letter-spacing: -.01em; }
  .subtitle { color: var(--text-dim); font-size: 14px; margin: 0; }

  .card {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 16px;
    box-shadow: var(--shadow);
    padding: 16px;
    margin-bottom: 14px;
  }

  .row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 12px;
  }
  .row h2 { font-size: 13px; text-transform: uppercase; letter-spacing: .08em; color: var(--text-dim); margin: 0; font-weight: 600; }

  .icon-btn {
    background: transparent;
    border: 1px solid var(--border);
    color: var(--text);
    border-radius: 10px;
    width: 34px; height: 34px;
    display: inline-flex; align-items: center; justify-content: center;
    cursor: pointer;
    transition: background .15s, transform .15s;
  }
  .icon-btn:hover { background: var(--bg-elev); }
  .icon-btn:active { transform: scale(.94); }
  .icon-btn svg { width: 16px; height: 16px; }
  .icon-btn.spinning svg { animation: spin .8s linear infinite; }
  @keyframes spin { to { transform: rotate(360deg); } }

  ul.networks {
    list-style: none;
    margin: 0; padding: 0;
    max-height: 320px;
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
    border-radius: 10px;
    background: var(--bg-elev);
  }
  ul.networks li {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 12px 14px;
    border-bottom: 1px solid var(--border);
    cursor: pointer;
    transition: background .12s;
    user-select: none;
  }
  ul.networks li:last-child { border-bottom: none; }
  ul.networks li:hover { background: var(--bg-card); }
  ul.networks li.selected {
    background: linear-gradient(90deg, rgba(91,140,255,.18), transparent);
    box-shadow: inset 3px 0 0 var(--accent);
  }
  .ssid { flex: 1; font-weight: 500; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .rssi {
    width: 28px;
    display: inline-flex;
    align-items: flex-end;
    gap: 2px;
    height: 16px;
  }
  .rssi span {
    width: 4px;
    background: var(--text-dim);
    opacity: .35;
    border-radius: 1px;
  }
  .rssi span:nth-child(1) { height: 4px; }
  .rssi span:nth-child(2) { height: 8px; }
  .rssi span:nth-child(3) { height: 12px; }
  .rssi span:nth-child(4) { height: 16px; }
  .rssi[data-bars="1"] span:nth-child(-n+1),
  .rssi[data-bars="2"] span:nth-child(-n+2),
  .rssi[data-bars="3"] span:nth-child(-n+3),
  .rssi[data-bars="4"] span:nth-child(-n+4) { opacity: 1; background: var(--accent); }

  .empty, .loading {
    color: var(--text-dim);
    text-align: center;
    padding: 28px 16px;
    font-size: 14px;
  }
  .spinner {
    width: 22px; height: 22px;
    border: 2px solid var(--border);
    border-top-color: var(--accent);
    border-radius: 50%;
    animation: spin .8s linear infinite;
    display: inline-block;
    vertical-align: middle;
    margin-right: 8px;
  }

  .field { position: relative; }
  input[type="password"], input[type="text"] {
    width: 100%;
    padding: 12px 44px 12px 14px;
    background: var(--bg-elev);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 12px;
    font-size: 16px;
    outline: none;
    transition: border-color .15s, box-shadow .15s;
  }
  input:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(91,140,255,.18); }
  .toggle-eye {
    position: absolute;
    right: 6px; top: 50%;
    transform: translateY(-50%);
    background: transparent;
    border: none;
    cursor: pointer;
    color: var(--text-dim);
    width: 36px; height: 36px;
    display: flex; align-items: center; justify-content: center;
  }
  .toggle-eye svg { width: 18px; height: 18px; }

  button.primary {
    width: 100%;
    padding: 14px;
    margin-top: 14px;
    border: none;
    border-radius: 12px;
    background: linear-gradient(135deg, var(--accent), #8a5bff);
    color: white;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
    transition: transform .12s, box-shadow .15s, opacity .15s;
    box-shadow: 0 6px 18px rgba(91,140,255,.35);
  }
  button.primary:hover { box-shadow: 0 8px 22px rgba(91,140,255,.45); }
  button.primary:active { transform: translateY(1px); }
  button.primary:disabled { opacity: .55; cursor: not-allowed; box-shadow: none; }

  .status {
    margin-top: 14px;
    padding: 12px 14px;
    border-radius: 10px;
    font-size: 14px;
    display: none;
    align-items: center;
    gap: 8px;
  }
  .status.show { display: flex; }
  .status.info    { background: rgba(91,140,255,.12); color: var(--accent); }
  .status.success { background: rgba(61,220,132,.12); color: var(--success); }
  .status.error   { background: rgba(255,107,107,.12); color: var(--error); }

  footer { text-align: center; color: var(--text-dim); font-size: 12px; margin-top: 18px; }
</style>
</head>
<body>
<main>
  <header>
    <div class="logo" aria-hidden="true">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M5 12.55a11 11 0 0 1 14.08 0"/>
        <path d="M1.42 9a16 16 0 0 1 21.16 0"/>
        <path d="M8.53 16.11a6 6 0 0 1 6.95 0"/>
        <line x1="12" y1="20" x2="12.01" y2="20"/>
      </svg>
    </div>
    <h1>Rover Wi-Fi Setup</h1>
    <p class="subtitle">Choose a network to connect your rover</p>
  </header>

  <section class="card">
    <div class="row">
      <h2>Available Networks</h2>
      <button id="refreshBtn" class="icon-btn" title="Refresh" aria-label="Refresh networks">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
          <polyline points="23 4 23 10 17 10"/>
          <polyline points="1 20 1 14 7 14"/>
          <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
        </svg>
      </button>
    </div>
    <ul id="networks" class="networks">
      <li class="loading"><span class="spinner"></span>Scanning...</li>
    </ul>
  </section>

  <section class="card">
    <div class="row">
      <h2>Password</h2>
    </div>
    <div class="field">
      <input type="password" id="password" placeholder="Enter Wi-Fi password" autocomplete="off" autocapitalize="off" autocorrect="off" spellcheck="false">
      <button type="button" class="toggle-eye" id="toggleEye" aria-label="Toggle password visibility">
        <svg id="eyeIcon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
          <circle cx="12" cy="12" r="3"/>
        </svg>
      </button>
    </div>
  </section>

  <section class="card">
    <button id="connectBtn" class="primary" disabled>Connect</button>
    <div id="status" class="status"></div>
  </section>

  <footer>Rover &middot; Captive Setup</footer>
</main>

<script>
(() => {
  const networksEl  = document.getElementById('networks');
  const refreshBtn  = document.getElementById('refreshBtn');
  const passwordEl  = document.getElementById('password');
  const toggleEye   = document.getElementById('toggleEye');
  const connectBtn  = document.getElementById('connectBtn');
  const statusEl    = document.getElementById('status');

  let selectedSsid = null;

  function rssiBars(rssi) {
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    return 1;
  }

  function setStatus(kind, text) {
    statusEl.className = 'status show ' + kind;
    statusEl.textContent = text;
  }
  function clearStatus() { statusEl.className = 'status'; statusEl.textContent = ''; }

  function renderEmpty() {
    networksEl.innerHTML = '<li class="empty">No networks found.<br>Tap refresh to try again.</li>';
  }
  function renderLoading() {
    networksEl.innerHTML = '<li class="loading"><span class="spinner"></span>Scanning...</li>';
  }

  function renderNetworks(list) {
    if (!list || !list.length) { renderEmpty(); return; }
    // de-dupe by SSID, keep strongest RSSI
    const seen = new Map();
    list.forEach(n => {
      if (!n.SSID) return;
      const prev = seen.get(n.SSID);
      if (!prev || n.RSSI > prev.RSSI) seen.set(n.SSID, n);
    });
    const uniq = [...seen.values()].sort((a,b) => b.RSSI - a.RSSI);

    networksEl.innerHTML = '';
    uniq.forEach(n => {
      const li = document.createElement('li');
      li.dataset.ssid = n.SSID;
      const bars = rssiBars(n.RSSI);
      li.innerHTML = `
        <span class="ssid"></span>
        <span class="rssi" data-bars="${bars}" title="${n.RSSI} dBm">
          <span></span><span></span><span></span><span></span>
        </span>`;
      li.querySelector('.ssid').textContent = n.SSID;
      li.addEventListener('click', () => selectNetwork(n.SSID, li));
      networksEl.appendChild(li);
    });
  }

  function selectNetwork(ssid, el) {
    selectedSsid = ssid;
    [...networksEl.children].forEach(c => c.classList.remove('selected'));
    el.classList.add('selected');
    connectBtn.disabled = false;
    passwordEl.focus();
  }

  function scan() {
    refreshBtn.classList.add('spinning');
    renderLoading();
    fetch('/scan')
      .then(r => r.json())
      .then(renderNetworks)
      .catch(() => { networksEl.innerHTML = '<li class="empty">Scan failed. Try again.</li>'; })
      .finally(() => refreshBtn.classList.remove('spinning'));
  }

  function connect() {
    if (!selectedSsid) { setStatus('error', 'Pick a network first.'); return; }

    connectBtn.disabled = true;
    setStatus('info', 'Connecting to "' + selectedSsid + '"...');

    fetch('/connect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'ssid=' + encodeURIComponent(selectedSsid) +
            '&password=' + encodeURIComponent(passwordEl.value)
    })
    .then(r => {
      if (r.ok) {
        setStatus('success', 'Connected. The rover will reboot...');
        setTimeout(() => { try { window.close(); } catch(e){} }, 2500);
      } else {
        connectBtn.disabled = false;
        setStatus('error', 'Connection failed. Check the password.');
      }
    })
    .catch(() => {
      connectBtn.disabled = false;
      setStatus('error', 'Network error.');
    });
  }

  toggleEye.addEventListener('click', () => {
    const isPwd = passwordEl.type === 'password';
    passwordEl.type = isPwd ? 'text' : 'password';
  });
  refreshBtn.addEventListener('click', scan);
  connectBtn.addEventListener('click', connect);
  passwordEl.addEventListener('keydown', e => { if (e.key === 'Enter') connect(); });

  scan();
})();
</script>
</body>
</html>
)rawliteral";

#endif
