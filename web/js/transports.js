// Transports panel controller — fetches the firmware's transport list and
// renders a row per transport with a toggle. Toggling sends
// `{command:"set_transport", name, enabled}`.
//
// The page itself talks WS, so toggling off `ws` would cut our own legs out
// from under us — that row's toggle is force-disabled. Other rows toggle
// freely; firmware persists nothing, so a reboot restores boot defaults.

const SELF_TRANSPORT = 'ws';   // the pipe this page uses

// Friendly labels for the firmware's short names. Anything not in this
// map falls back to the raw name so a future transport renders sanely
// without a UI change.
const LABELS = {
  http:   'HTTP',
  ws:     'WebSocket',
  telnet: 'Telnet',
  serial: 'Serial',
  espnow: 'ESP-NOW',
  mqtt:   'MQTT',
};

const DESCRIPTIONS = {
  http:   'POST /api/cmd on port 32231',
  ws:     'WebSocket on port 32232 (this page)',
  telnet: 'Telnet on port 23 — logs + commands',
  serial: 'JSON-per-line over UART0',
  espnow: '2.4 GHz peer broadcast (no Wi-Fi)',
  mqtt:   'rover/<id>/{cmd,response,telemetry}',
};

export function labelFor(name)       { return LABELS[name] || name; }
export function descriptionFor(name) { return DESCRIPTIONS[name] || ''; }

// Fetch the current transport list from firmware.
export async function fetchTransports(send) {
  const reply = await send({ command: 'transports' });
  if (reply?.status) throw new Error(reply.status);
  if (!Array.isArray(reply?.response)) {
    throw new Error('unexpected reply shape');
  }
  return reply.response;
}

// Toggle a single transport. Refuses to toggle the page's own pipe — that
// would close the connection and orphan the UI.
export async function toggleTransport(send, name, enabled) {
  if (name === SELF_TRANSPORT && !enabled) {
    throw new Error('cannot disable the active control transport');
  }
  const reply = await send({ command: 'set_transport', name, enabled });
  if (reply?.status) throw new Error(reply.status);
  return reply?.response;
}

export function isSelfTransport(name) { return name === SELF_TRANSPORT; }

// ---------------------------------------------------------------------------
// DOM rendering — kept in this file so the test suite can exercise it via
// jsdom without dragging in control.js's full WS lifecycle.

export function renderTransports(container, transports, { onToggle } = {}) {
  container.innerHTML = '';
  if (!transports.length) {
    const empty = document.createElement('div');
    empty.className = 'tr-empty';
    empty.textContent = 'No transports registered.';
    container.appendChild(empty);
    return;
  }
  for (const t of transports) {
    const row = document.createElement('div');
    row.className = 'tr-row';
    row.dataset.name = t.name;

    const left = document.createElement('div');
    left.className = 'tr-meta';
    const label = document.createElement('div');
    label.className = 'tr-label';
    label.textContent = labelFor(t.name);
    if (isSelfTransport(t.name)) {
      const tag = document.createElement('span');
      tag.className = 'tr-tag';
      tag.textContent = 'in use';
      label.appendChild(tag);
    }
    const desc = document.createElement('div');
    desc.className = 'tr-desc';
    desc.textContent = descriptionFor(t.name);
    left.appendChild(label);
    left.appendChild(desc);

    const toggle = document.createElement('label');
    toggle.className = 'tr-switch';
    const cb = document.createElement('input');
    cb.type = 'checkbox';
    cb.checked = !!t.running;
    cb.disabled = isSelfTransport(t.name);   // can't disable our own pipe
    cb.dataset.name = t.name;
    cb.addEventListener('change', () => onToggle?.(t.name, cb.checked, cb));
    const slider = document.createElement('span');
    slider.className = 'tr-slider';
    toggle.appendChild(cb);
    toggle.appendChild(slider);

    row.appendChild(left);
    row.appendChild(toggle);
    container.appendChild(row);
  }
}
