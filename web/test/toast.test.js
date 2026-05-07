// Tests for the toast notification helper. toast.js holds module-level
// state (the cached element + timer), so each test re-imports the
// module after resetting the DOM to start clean.
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';

let toast;
beforeEach(async () => {
  document.body.innerHTML = '';
  vi.useFakeTimers();
  vi.resetModules();
  ({ toast } = await import('../js/toast.js'));
});
afterEach(() => {
  vi.useRealTimers();
});

describe('toast', () => {
  it('creates a single .toast element on first call', () => {
    toast('hello');
    const nodes = document.querySelectorAll('.toast');
    expect(nodes.length).toBe(1);
    expect(nodes[0].textContent).toBe('hello');
    expect(nodes[0].className).toContain('show');
  });

  it('reuses the same element on subsequent calls', () => {
    toast('first');
    const first = document.querySelector('.toast');
    toast('second');
    const second = document.querySelector('.toast');
    expect(first).toBe(second);
    expect(second.textContent).toBe('second');
  });

  it('applies the kind class', () => {
    toast('warn', 'warning');
    expect(document.querySelector('.toast').className).toContain('warning');
  });

  it('clears show class after 2400 ms', () => {
    toast('temp');
    expect(document.querySelector('.toast').className).toContain('show');
    vi.advanceTimersByTime(2399);
    expect(document.querySelector('.toast').className).toContain('show');
    vi.advanceTimersByTime(1);
    expect(document.querySelector('.toast').className).not.toContain('show');
  });

  it('rapid successive calls reset the dismiss timer', () => {
    toast('a');
    vi.advanceTimersByTime(2000);
    toast('b');
    vi.advanceTimersByTime(2000);
    expect(document.querySelector('.toast').className).toContain('show');
    vi.advanceTimersByTime(500);
    expect(document.querySelector('.toast').className).not.toContain('show');
  });
});
