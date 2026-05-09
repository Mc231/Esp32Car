// Per-session telemetry logger. Captures every event the page produces
// (autonomous decisions, motor commands, vision samples, manual inputs,
// recorder events) and ships them in batches to the local server, which
// persists them to SQLite for offline analysis.
//
// Designed to fail open: if the server isn't running (e.g. the user
// launched the page from a static host), the logger silently buffers
// in memory and never throws into caller paths.
//
// Lifecycle:
//   const log = createLogger({ host, name });
//   await log.start();           // POST /api/sessions
//   log.event('tick', { ... });  // buffered
//   ... (events flushed every FLUSH_INTERVAL_MS)
//   await log.end();             // POST /api/sessions/<id>/end + final flush
//
// Time semantics: events use t_ms = ms since session.start(). The server
// already records the wall-clock session start, so this keeps the
// per-event field small and trivially relative.

const FLUSH_INTERVAL_MS = 3000;
const MAX_BATCH         = 500;
const POST_TIMEOUT_MS   = 4000;

export function createLogger({
  host,
  name,
  serverBase = '',          // '' = same origin (the page's own server)
  fetchImpl  = (typeof fetch !== 'undefined' ? fetch.bind(globalThis) : null),
  now        = (typeof performance !== 'undefined' ? () => performance.now() : Date.now),
} = {}) {
  if (!host) throw new Error('createLogger: host required');

  const buffer  = [];
  let sessionId = null;
  let sessionStartedAt = 0;
  let flushTimer = null;
  let started = false;
  let dead    = false;        // server is unreachable; stop trying

  function tNow() {
    return Math.max(0, Math.round(now() - sessionStartedAt));
  }

  async function start() {
    if (started) return sessionId;
    started = true;
    sessionStartedAt = now();
    if (!fetchImpl) { dead = true; return null; }
    try {
      const res = await postJson('/api/sessions', {
        rover_host: host,
        rover_name: name || host,
        user_agent: typeof navigator !== 'undefined' ? navigator.userAgent : '',
      });
      sessionId = res?.id ?? null;
      if (sessionId == null) { dead = true; return null; }
      console.log(`[logger] session #${sessionId} started`);
      flushTimer = setInterval(flush, FLUSH_INTERVAL_MS);
      return sessionId;
    } catch (e) {
      // Server probably isn't running. Disable, keep working silently.
      console.warn('[logger] server unreachable — logging disabled:', e.message);
      dead = true;
      return null;
    }
  }

  function event(type, payload) {
    if (dead || !started) return;
    if (!type) return;
    buffer.push({ t_ms: tNow(), type, payload: payload ?? null });
    if (buffer.length >= MAX_BATCH) flush();
  }

  async function flush() {
    if (dead || sessionId == null) return;
    if (buffer.length === 0)       return;
    const batch = buffer.splice(0, buffer.length);
    try {
      await postJson(`/api/sessions/${sessionId}/events`, batch);
    } catch (e) {
      // Server died mid-session. Re-buffer this batch at the front so
      // we don't lose it, but don't keep retrying forever — give up
      // after the next attempt also fails.
      buffer.unshift(...batch);
      if (!dead) {
        console.warn('[logger] flush failed:', e.message);
      }
      // Probe: if buffer is well above the cap, drop the oldest to
      // bound memory.
      if (buffer.length > MAX_BATCH * 4) {
        buffer.splice(0, buffer.length - MAX_BATCH * 4);
        console.warn('[logger] event buffer overflow — dropping oldest');
      }
    }
  }

  async function end() {
    if (!started || dead || sessionId == null) return;
    if (flushTimer) { clearInterval(flushTimer); flushTimer = null; }
    await flush();
    try { await postJson(`/api/sessions/${sessionId}/end`, null); }
    catch { /* best-effort */ }
  }

  // Capture a camera frame alongside an event, saving the image
  // server-side for offline review. Logs the event into the regular
  // events table (so SQL queries still see it) and POSTs the JPEG to
  // /snapshots — the server JOINs them later by (session_id, t_ms, type).
  async function snapshotEvent(type, payload, jpegBlob) {
    if (dead || !started || sessionId == null) return;
    if (!type)     return;
    if (!jpegBlob) return event(type, payload);   // fall back to event-only
    // Snapshot and event MUST share t_ms exactly so the SQL JOIN by
    // (session_id, t_ms, type) resolves cleanly. Build the event
    // record by hand instead of calling event() (which would compute
    // its own tNow() a few ms later).
    const t_ms = tNow();
    buffer.push({ t_ms, type, payload: payload ?? null });
    if (buffer.length >= MAX_BATCH) flush();
    if (!fetchImpl) return;
    try {
      const qs = `?t_ms=${t_ms}&type=${encodeURIComponent(type)}`;
      const ctl = (typeof AbortController !== 'undefined') ? new AbortController() : null;
      const tmr = ctl ? setTimeout(() => ctl.abort(), POST_TIMEOUT_MS * 2) : null;
      try {
        await fetchImpl(`${serverBase}/api/sessions/${sessionId}/snapshots${qs}`, {
          method: 'POST',
          headers: { 'Content-Type': 'image/jpeg' },
          body: jpegBlob,
          signal: ctl ? ctl.signal : undefined,
        });
      } finally {
        if (tmr) clearTimeout(tmr);
      }
    } catch (e) {
      console.warn('[logger] snapshot upload failed:', e.message);
    }
  }

  // Used on unload — sendBeacon so the browser commits the request even
  // if the page is closing.
  function flushBeacon() {
    if (dead || sessionId == null || buffer.length === 0) return;
    if (typeof navigator === 'undefined' || !navigator.sendBeacon) return;
    const blob = new Blob([JSON.stringify(buffer)], { type: 'application/json' });
    if (navigator.sendBeacon(`${serverBase}/api/sessions/${sessionId}/events`, blob)) {
      buffer.length = 0;
    }
  }

  async function postJson(path, body) {
    if (!fetchImpl) throw new Error('no fetch');
    const ctl = (typeof AbortController !== 'undefined') ? new AbortController() : null;
    const t   = ctl ? setTimeout(() => ctl.abort(), POST_TIMEOUT_MS) : null;
    try {
      const res = await fetchImpl(`${serverBase}${path}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: body == null ? '' : JSON.stringify(body),
        signal: ctl ? ctl.signal : undefined,
      });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      // 204 has no body; only parse JSON when there's content.
      const ct = res.headers?.get?.('content-type') || '';
      return ct.includes('application/json') ? await res.json() : null;
    } finally {
      if (t) clearTimeout(t);
    }
  }

  return {
    start, event, snapshotEvent, flush, end, flushBeacon,
    isActive: () => started && !dead && sessionId != null,
    sessionId: () => sessionId,
    bufferedCount: () => buffer.length,
  };
}
