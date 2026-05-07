// Tests for the localStorage-backed rover list.
import { describe, it, expect, beforeEach } from 'vitest';
import { loadRovers, saveRovers, addRover, removeRover, renameRover } from '../js/store.js';

const KEY = 'rovers.v1';

describe('store', () => {
  beforeEach(() => { localStorage.clear(); });

  it('loadRovers returns [] when storage is empty', () => {
    expect(loadRovers()).toEqual([]);
  });

  it('loadRovers returns [] when storage has invalid JSON', () => {
    localStorage.setItem(KEY, 'not json');
    expect(loadRovers()).toEqual([]);
  });

  it('saveRovers persists and loadRovers reads it back', () => {
    const list = [{ name: 'A', host: '1.2.3.4' }, { name: 'B', host: 'b.local' }];
    saveRovers(list);
    expect(loadRovers()).toEqual(list);
  });

  it('addRover appends a new entry', () => {
    const r = addRover('Garage', '192.168.1.42');
    expect(r).toEqual({ ok: true });
    expect(loadRovers()).toEqual([{ name: 'Garage', host: '192.168.1.42' }]);
  });

  it('addRover refuses duplicates by host', () => {
    addRover('A', '1.1.1.1');
    const r = addRover('A2', '1.1.1.1');
    expect(r).toEqual({ ok: false, reason: 'exists' });
    expect(loadRovers().length).toBe(1);
  });

  it('addRover allows same name with different host', () => {
    addRover('Rover', 'a.local');
    const r = addRover('Rover', 'b.local');
    expect(r).toEqual({ ok: true });
    expect(loadRovers().length).toBe(2);
  });

  it('removeRover splices by index', () => {
    addRover('A', '1');
    addRover('B', '2');
    addRover('C', '3');
    removeRover(1);
    expect(loadRovers()).toEqual([
      { name: 'A', host: '1' },
      { name: 'C', host: '3' },
    ]);
  });

  it('renameRover updates name only, preserves host', () => {
    addRover('Old', '5.5.5.5');
    renameRover(0, 'New');
    expect(loadRovers()).toEqual([{ name: 'New', host: '5.5.5.5' }]);
  });

  it('renameRover is a no-op for invalid index', () => {
    addRover('A', '1');
    renameRover(99, 'X');
    expect(loadRovers()).toEqual([{ name: 'A', host: '1' }]);
  });
});
