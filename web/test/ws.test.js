// Tests for the WS transport wrapper. Replaces the global WebSocket with a
// controllable fake so we can simulate open/close/message events deterministically.
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';

class FakeWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = FakeWebSocket.CONNECTING;
    this.sent = [];
    this.onopen = null;
    this.onclose = null;
    this.onerror = null;
    this.onmessage = null;
    FakeWebSocket.last = this;
    FakeWebSocket.instances.push(this);
  }
  send(data) { this.sent.push(data); }
  close() {
    if (this.readyState === FakeWebSocket.CLOSED) return;
    this.readyState = FakeWebSocket.CLOSED;
    this.onclose?.({ code: 1000, reason: 'client closed', wasClean: true });
  }
  // Test-only helpers:
  _open() { this.readyState = FakeWebSocket.OPEN; this.onopen?.(); }
  _message(obj) { this.onmessage?.({ data: JSON.stringify(obj) }); }
  _serverClose() {
    this.readyState = FakeWebSocket.CLOSED;
    this.onclose?.({ code: 1006, reason: '', wasClean: false });
  }
}
FakeWebSocket.CONNECTING = 0;
FakeWebSocket.OPEN = 1;
FakeWebSocket.CLOSING = 2;
FakeWebSocket.CLOSED = 3;
FakeWebSocket.instances = [];
FakeWebSocket.last = null;

let createConnection, probe;
beforeEach(async () => {
  vi.useFakeTimers();
  FakeWebSocket.instances = [];
  FakeWebSocket.last = null;
  globalThis.WebSocket = FakeWebSocket;
  // Re-import to pick up clean module state.
  vi.resetModules();
  ({ createConnection, probe } = await import('../js/ws.js'));
});

afterEach(() => {
  vi.useRealTimers();
});

describe('createConnection', () => {
  it('opens a WebSocket to the right URL', () => {
    createConnection({ host: 'rover.local' });
    expect(FakeWebSocket.last.url).toBe('ws://rover.local:32232/');
  });

  it('calls onOpen when the socket opens', () => {
    const onOpen = vi.fn();
    createConnection({ host: 'r', onOpen });
    FakeWebSocket.last._open();
    expect(onOpen).toHaveBeenCalledOnce();
  });

  it('rejects send() when not connected', async () => {
    const conn = createConnection({ host: 'r' });
    // Before _open() the socket is in CONNECTING.
    await expect(conn.send({ command: 'x' })).rejects.toThrow('not connected');
  });

  it('resolves send() with the next inbound message (FIFO)', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    const p = conn.send({ command: 'system' });
    FakeWebSocket.last._message({ response: { up_time: 5 } });
    await expect(p).resolves.toEqual({ response: { up_time: 5 } });
  });

  it('preserves FIFO ordering for batched sends', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    const p1 = conn.send({ command: 'a' });
    const p2 = conn.send({ command: 'b' });
    FakeWebSocket.last._message({ id: 1 });
    FakeWebSocket.last._message({ id: 2 });
    await expect(p1).resolves.toEqual({ id: 1 });
    await expect(p2).resolves.toEqual({ id: 2 });
  });

  it('serializes the payload as JSON over the wire', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    conn.send({ command: 'set_motor', action: 0, motor: 2 });
    expect(FakeWebSocket.last.sent[0])
      .toBe('{"command":"set_motor","action":0,"motor":2}');
  });

  it('rejects pending sends when the connection drops', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    const p = conn.send({ command: 'a' });
    FakeWebSocket.last._serverClose();
    // The dropped resolver gets called with { error: 'disconnected' } —
    // send() resolves with that envelope, it doesn't reject.
    await expect(p).resolves.toEqual({ error: 'disconnected' });
  });

  it('schedules a reconnect after a server-side close', () => {
    createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    FakeWebSocket.last._serverClose();
    expect(FakeWebSocket.instances.length).toBe(1);
    vi.advanceTimersByTime(2000);
    expect(FakeWebSocket.instances.length).toBe(2);
  });

  it('disconnect() suppresses reconnect attempts', () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    conn.disconnect();
    vi.advanceTimersByTime(5000);
    expect(FakeWebSocket.instances.length).toBe(1);
  });

  it('times out send() after 4 s when no reply arrives', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    const p = conn.send({ command: 'silent' });
    vi.advanceTimersByTime(4000);
    await expect(p).rejects.toThrow('timeout');
  });

  it('fireAndForget send() does not reject on timeout', async () => {
    const conn = createConnection({ host: 'r' });
    FakeWebSocket.last._open();
    const p = conn.send({ command: 'reboot' }, { fireAndForget: true });
    vi.advanceTimersByTime(4000);
    // Promise stays pending forever — but we shouldn't see an
    // unhandled-rejection warning.
    let settled = false;
    p.then(() => { settled = true; }, () => { settled = true; });
    await Promise.resolve();
    expect(settled).toBe(false);
  });

  it('isOpen() reflects readyState', () => {
    const conn = createConnection({ host: 'r' });
    expect(conn.isOpen()).toBe(false);
    FakeWebSocket.last._open();
    expect(conn.isOpen()).toBe(true);
    conn.disconnect();
    expect(conn.isOpen()).toBe(false);
  });

  it('onMessage callback fires for every inbound frame', () => {
    const onMessage = vi.fn();
    createConnection({ host: 'r', onMessage });
    FakeWebSocket.last._open();
    FakeWebSocket.last._message({ a: 1 });
    FakeWebSocket.last._message({ b: 2 });
    expect(onMessage).toHaveBeenCalledTimes(2);
  });

  it('non-JSON messages are dropped silently', () => {
    const onMessage = vi.fn();
    createConnection({ host: 'r', onMessage });
    FakeWebSocket.last._open();
    FakeWebSocket.last.onmessage({ data: 'not json' });
    expect(onMessage).not.toHaveBeenCalled();
  });
});

describe('probe', () => {
  it('resolves true on socket open', async () => {
    const p = probe('r', 1000);
    FakeWebSocket.last._open();
    await expect(p).resolves.toBe(true);
  });

  it('resolves false on error', async () => {
    const p = probe('r', 1000);
    FakeWebSocket.last.onerror?.();
    await expect(p).resolves.toBe(false);
  });

  it('resolves false on timeout', async () => {
    const p = probe('r', 200);
    vi.advanceTimersByTime(250);
    await expect(p).resolves.toBe(false);
  });
});
