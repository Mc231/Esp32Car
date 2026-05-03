// Thin WebSocket wrapper for talking to the rover's WS server (port 32232).
//
// The protocol is request → single reply per send, with no correlation id.
// We rely on the server replying in send order and queue resolvers in FIFO,
// which works as long as the page has a single in-flight `send()` chain at
// a time (i.e. don't call `send` before awaiting the previous send's reply
// unless you use Promise.all to send a batch in lockstep).
//
// Usage:
//   const conn = createConnection({ host, onOpen, onClose, onMessage });
//   await conn.send({ command: 'system' });
//   conn.send({ command: 'reboot' }, { fireAndForget: true });

const WS_PORT = 32232;
const REPLY_TIMEOUT_MS = 4000;

export function createConnection({ host, onOpen, onClose, onMessage } = {}) {
  let ws = null;
  const pending = new Map();         // FIFO queue of pending resolvers
  let reconnectTimer = null;
  let alive = true;                  // false when caller calls disconnect()

  function connect() {
    const url = `ws://${host}:${WS_PORT}/`;
    console.log('[ws] connecting to', url);
    ws = new WebSocket(url);
    ws.onopen = () => {
      console.log('[ws] open');
      onOpen?.();
    };
    ws.onclose = ev => {
      console.warn(`[ws] close — code=${ev.code} reason="${ev.reason}" wasClean=${ev.wasClean}`);
      // Reject any in-flight requests so callers don't hang forever.
      pending.forEach(cb => { try { cb({ error: 'disconnected' }); } catch {} });
      pending.clear();
      ws = null;
      onClose?.();
      if (alive) {
        clearTimeout(reconnectTimer);
        reconnectTimer = setTimeout(connect, 2000);
      }
    };
    ws.onerror = e => console.error('[ws] error', e);
    ws.onmessage = ev => {
      let msg;
      try { msg = JSON.parse(ev.data); }
      catch { console.warn('[ws] non-JSON:', ev.data); return; }
      console.log('[ws] <<', JSON.stringify(msg));
      onMessage?.(msg);
      const oldest = pending.size ? pending.entries().next().value : null;
      if (oldest) {
        const [key, cb] = oldest;
        pending.delete(key);
        cb(msg);
      } else {
        console.log('[ws] reply with no pending resolver:', msg);
      }
    };
  }

  function send(payload, { fireAndForget = false } = {}) {
    return new Promise((resolve, reject) => {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        reject(new Error('not connected'));
        return;
      }
      // We always queue a resolver — even for fire-and-forget — so the FIFO
      // queue stays in sync with the server's reply order. The fire-and-forget
      // caller just doesn't await the result.
      const key = Symbol();
      pending.set(key, msg => resolve(msg));
      setTimeout(() => {
        if (pending.has(key)) {
          pending.delete(key);
          if (!fireAndForget) reject(new Error('timeout'));
        }
      }, REPLY_TIMEOUT_MS);
      try {
        const text = JSON.stringify(payload);
        console.log('[ws] >>', text);
        ws.send(text);
      } catch (e) { console.error('[ws] send threw', e); reject(e); }
    });
  }

  function disconnect() {
    alive = false;
    clearTimeout(reconnectTimer);
    if (ws) try { ws.close(); } catch {}
    ws = null;
  }

  function isOpen() { return !!ws && ws.readyState === WebSocket.OPEN; }

  connect();
  return { send, disconnect, isOpen };
}

// Quick "is this rover alive?" probe — opens a WS, waits for either open
// or error, closes, returns true/false. Used by the picker to render
// online/offline pills.
export function probe(host, timeoutMs = 2500) {
  return new Promise(resolve => {
    let done = false;
    let ws;
    try { ws = new WebSocket(`ws://${host}:${WS_PORT}/`); }
    catch { resolve(false); return; }
    const timer = setTimeout(() => {
      if (done) return; done = true;
      try { ws.close(); } catch {}
      resolve(false);
    }, timeoutMs);
    const finish = ok => {
      if (done) return; done = true;
      clearTimeout(timer);
      try { ws.close(); } catch {}
      resolve(ok);
    };
    ws.onopen  = () => finish(true);
    ws.onerror = () => finish(false);
    ws.onclose = () => finish(false);
  });
}
