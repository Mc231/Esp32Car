// Vitest setup: install a working localStorage shim. Node 25 ships a partial
// experimental localStorage that lacks .clear() and conflicts with jsdom's;
// we just replace it with a Map-backed Storage-compatible object.
import { vi } from 'vitest';

class MemoryStorage {
  constructor() { this.map = new Map(); }
  get length() { return this.map.size; }
  key(i) { return Array.from(this.map.keys())[i] ?? null; }
  getItem(k) { return this.map.has(k) ? this.map.get(k) : null; }
  setItem(k, v) { this.map.set(String(k), String(v)); }
  removeItem(k) { this.map.delete(k); }
  clear() { this.map.clear(); }
}

vi.stubGlobal('localStorage', new MemoryStorage());
