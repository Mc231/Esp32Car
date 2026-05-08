// Tests for the transports controller — fetching, toggling, and rendering.
// jsdom gives us a real DOM so renderTransports can be exercised without
// the full control.js page bringing up a WebSocket.
import { describe, it, expect, vi, beforeEach } from 'vitest';
import {
  fetchTransports,
  toggleTransport,
  renderTransports,
  labelFor,
  descriptionFor,
  isSelfTransport,
} from '../js/transports.js';

const STUB_LIST = [
  { name: 'http',   running: true  },
  { name: 'ws',     running: true  },
  { name: 'telnet', running: false },
  { name: 'mqtt',   running: false },
];

function makeSend(reply) {
  return vi.fn().mockResolvedValue(reply);
}

describe('labelFor / descriptionFor', () => {
  it('returns friendly names for known transports', () => {
    expect(labelFor('ws')).toBe('WebSocket');
    expect(labelFor('espnow')).toBe('ESP-NOW');
    expect(labelFor('mqtt')).toBe('MQTT');
  });

  it('falls back to the raw name for unknown transports', () => {
    expect(labelFor('quic')).toBe('quic');
    expect(descriptionFor('quic')).toBe('');
  });
});

describe('isSelfTransport', () => {
  it('only ws is the page-owned pipe', () => {
    expect(isSelfTransport('ws')).toBe(true);
    expect(isSelfTransport('http')).toBe(false);
    expect(isSelfTransport('mqtt')).toBe(false);
  });
});

describe('fetchTransports', () => {
  it('returns the response array on success', async () => {
    const send = makeSend({ response: STUB_LIST });
    const result = await fetchTransports(send);
    expect(result).toEqual(STUB_LIST);
    expect(send).toHaveBeenCalledWith({ command: 'transports' });
  });

  it('throws when firmware returns a status (registry unavailable)', async () => {
    const send = makeSend({ status: 'transport registry unavailable' });
    await expect(fetchTransports(send)).rejects.toThrow(/unavailable/);
  });

  it('throws on unexpected reply shape', async () => {
    const send = makeSend({ response: 'not an array' });
    await expect(fetchTransports(send)).rejects.toThrow(/unexpected reply/);
  });
});

describe('toggleTransport', () => {
  it('sends the set_transport command and resolves with the response', async () => {
    const send = makeSend({ response: { name: 'mqtt', running: true } });
    const out = await toggleTransport(send, 'mqtt', true);
    expect(send).toHaveBeenCalledWith({
      command: 'set_transport', name: 'mqtt', enabled: true,
    });
    expect(out).toEqual({ name: 'mqtt', running: true });
  });

  it('refuses to disable the active ws transport', async () => {
    const send = makeSend({});
    await expect(toggleTransport(send, 'ws', false)).rejects.toThrow(/active control/);
    expect(send).not.toHaveBeenCalled();
  });

  it('allows enabling ws (no-op on running, but still safe)', async () => {
    const send = makeSend({ response: { name: 'ws', running: true } });
    await expect(toggleTransport(send, 'ws', true)).resolves.toBeTruthy();
  });

  it('throws when firmware returns a status (unknown transport)', async () => {
    const send = makeSend({ status: 'unknown transport' });
    await expect(toggleTransport(send, 'xyz', true)).rejects.toThrow(/unknown transport/);
  });
});

describe('renderTransports', () => {
  let container;
  beforeEach(() => {
    container = document.createElement('div');
    document.body.appendChild(container);
  });

  it('renders one row per transport', () => {
    renderTransports(container, STUB_LIST);
    expect(container.querySelectorAll('.tr-row').length).toBe(4);
  });

  it('marks ws row as the active pipe and disables its checkbox', () => {
    renderTransports(container, STUB_LIST);
    const wsRow = container.querySelector('.tr-row[data-name="ws"]');
    expect(wsRow).toBeTruthy();
    expect(wsRow.querySelector('.tr-tag')?.textContent).toBe('in use');
    expect(wsRow.querySelector('input[type="checkbox"]').disabled).toBe(true);
  });

  it('reflects running state in the checkbox', () => {
    renderTransports(container, STUB_LIST);
    expect(container.querySelector('.tr-row[data-name="http"] input').checked).toBe(true);
    expect(container.querySelector('.tr-row[data-name="mqtt"] input').checked).toBe(false);
  });

  it('invokes onToggle with the transport name and new state', () => {
    const onToggle = vi.fn();
    renderTransports(container, STUB_LIST, { onToggle });
    const cb = container.querySelector('.tr-row[data-name="mqtt"] input');
    cb.checked = true;
    cb.dispatchEvent(new Event('change'));
    expect(onToggle).toHaveBeenCalledWith('mqtt', true, cb);
  });

  it('renders a friendly empty state when the list is empty', () => {
    renderTransports(container, []);
    expect(container.querySelector('.tr-empty')?.textContent).toMatch(/No transports/);
  });

  it('falls back to the raw name when an unknown transport ships', () => {
    renderTransports(container, [{ name: 'quic', running: true }]);
    expect(container.querySelector('.tr-label').textContent).toContain('quic');
  });
});
