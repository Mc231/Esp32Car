// Tests for the session logger. Mocks fetch so we can assert on the
// request shape and timing. Vitest fake timers drive the periodic
// flush.
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { createLogger } from '../js/logger.js';

function mockFetch({ sessionId = 7, sessionsFails = false, eventsFails = false } = {}) {
  return vi.fn(async (url, opts) => {
    if (sessionsFails && url.endsWith('/api/sessions') && opts.method === 'POST') {
      throw new Error('connect refused');
    }
    if (eventsFails && url.includes('/events')) {
      throw new Error('write failed');
    }
    if (url.endsWith('/api/sessions')) {
      return {
        ok: true,
        headers: { get: () => 'application/json' },
        json: async () => ({ id: sessionId }),
      };
    }
    return { ok: true, headers: { get: () => '' } };
  });
}

describe('createLogger — startup', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('throws if host is missing', () => {
    expect(() => createLogger({})).toThrow(/host/);
  });

  it('opens a session and stores the returned id', async () => {
    const fetchImpl = mockFetch({ sessionId: 42 });
    const log = createLogger({ host: '1.2.3.4', name: 'Garage', fetchImpl });
    const id = await log.start();
    expect(id).toBe(42);
    expect(log.sessionId()).toBe(42);
    expect(log.isActive()).toBe(true);
    // First call was the session POST.
    expect(fetchImpl.mock.calls[0][0]).toBe('/api/sessions');
    expect(JSON.parse(fetchImpl.mock.calls[0][1].body)).toMatchObject({
      rover_host: '1.2.3.4',
      rover_name: 'Garage',
    });
  });

  it('falls back to host as the name when name is omitted', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: '1.2.3.4', fetchImpl });
    await log.start();
    expect(JSON.parse(fetchImpl.mock.calls[0][1].body).rover_name).toBe('1.2.3.4');
  });

  it('disables itself silently when the server is unreachable', async () => {
    const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const fetchImpl = mockFetch({ sessionsFails: true });
    const log = createLogger({ host: '1.2.3.4', fetchImpl });
    const id = await log.start();
    expect(id).toBeNull();
    expect(log.isActive()).toBe(false);
    log.event('tick', { x: 1 });   // should not throw
    expect(log.bufferedCount()).toBe(0);
    warn.mockRestore();
  });

  it('start() is idempotent', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: '1.2.3.4', fetchImpl });
    await log.start();
    await log.start();   // second call no-ops
    const sessionPosts = fetchImpl.mock.calls.filter(([u]) => u === '/api/sessions');
    expect(sessionPosts.length).toBe(1);
  });
});

describe('createLogger — buffering and flushing', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('buffers events without sending', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    log.event('tick', { d: 50 });
    log.event('tick', { d: 45 });
    expect(log.bufferedCount()).toBe(2);
    // Only the session POST happened so far.
    expect(fetchImpl.mock.calls.length).toBe(1);
  });

  it('drops events recorded before start()', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: 'r', fetchImpl });
    log.event('tick', { d: 99 });          // before start → discarded
    expect(log.bufferedCount()).toBe(0);
    await log.start();
    log.event('tick', { d: 50 });
    expect(log.bufferedCount()).toBe(1);
  });

  it('flushes buffered events to /api/sessions/<id>/events on the periodic timer', async () => {
    const fetchImpl = mockFetch({ sessionId: 11 });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    log.event('tick', { d: 50 });
    log.event('vision', { left: 0.1 });
    await vi.advanceTimersByTimeAsync(3000);   // FLUSH_INTERVAL_MS
    const flushCall = fetchImpl.mock.calls.find(([u]) => u === '/api/sessions/11/events');
    expect(flushCall).toBeDefined();
    const body = JSON.parse(flushCall[1].body);
    expect(body.length).toBe(2);
    expect(body[0]).toMatchObject({ type: 'tick' });
    expect(body[1]).toMatchObject({ type: 'vision' });
    expect(typeof body[0].t_ms).toBe('number');
    expect(log.bufferedCount()).toBe(0);
  });

  it('triggers an immediate flush when the buffer hits MAX_BATCH', async () => {
    const fetchImpl = mockFetch({ sessionId: 11 });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    for (let i = 0; i < 500; i++) log.event('tick', { i });
    // The 500th push should have triggered an immediate flush —
    // give the microtask a chance to land.
    await vi.advanceTimersByTimeAsync(0);
    expect(fetchImpl.mock.calls.some(([u]) => u.endsWith('/events'))).toBe(true);
  });

  it('event() with no type is silently ignored', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    log.event('', { x: 1 });
    log.event(undefined, { y: 2 });
    expect(log.bufferedCount()).toBe(0);
  });

  it('re-buffers a failed flush so events are not lost', async () => {
    const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const fetchImpl = mockFetch({ sessionId: 11, eventsFails: true });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    log.event('tick', { d: 50 });
    await vi.advanceTimersByTimeAsync(3000);
    // Flush failed → event remains in buffer.
    expect(log.bufferedCount()).toBe(1);
    warn.mockRestore();
  });
});

describe('createLogger — snapshotEvent', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('records the event AND uploads the JPEG body to /snapshots', async () => {
    const fetchImpl = mockFetch({ sessionId: 11 });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    const blob = new Blob(['jpeg-bytes'], { type: 'image/jpeg' });
    await log.snapshotEvent('pivot_start', { entryDistance: 12 }, blob);
    // Event was buffered.
    expect(log.bufferedCount()).toBe(1);
    // Snapshot was POSTed.
    const snapPost = fetchImpl.mock.calls.find(
      ([u]) => typeof u === 'string' && u.includes('/snapshots?')
    );
    expect(snapPost).toBeDefined();
    expect(snapPost[0]).toMatch(/\/api\/sessions\/11\/snapshots\?t_ms=\d+&type=pivot_start/);
    expect(snapPost[1].headers['Content-Type']).toBe('image/jpeg');
    expect(snapPost[1].body).toBe(blob);
  });

  it('falls back to event-only when no blob is supplied', async () => {
    const fetchImpl = mockFetch({ sessionId: 11 });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    await log.snapshotEvent('pivot_start', { x: 1 }, null);
    expect(log.bufferedCount()).toBe(1);
    const snapPost = fetchImpl.mock.calls.find(
      ([u]) => typeof u === 'string' && u.includes('/snapshots')
    );
    expect(snapPost).toBeUndefined();
  });

  it('is a no-op when the logger never started', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: 'r', fetchImpl });
    await log.snapshotEvent('pivot_start', {}, new Blob(['x']));
    expect(fetchImpl).not.toHaveBeenCalled();
  });
});

describe('createLogger — end()', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('flushes remaining events and POSTs /end', async () => {
    const fetchImpl = mockFetch({ sessionId: 11 });
    const log = createLogger({ host: 'r', fetchImpl });
    await log.start();
    log.event('tick', { d: 50 });
    await log.end();
    const calls = fetchImpl.mock.calls.map(([u]) => u);
    expect(calls).toContain('/api/sessions/11/events');
    expect(calls).toContain('/api/sessions/11/end');
  });

  it('is a no-op when the logger never started', async () => {
    const fetchImpl = mockFetch();
    const log = createLogger({ host: 'r', fetchImpl });
    await log.end();
    expect(fetchImpl).not.toHaveBeenCalled();
  });
});
