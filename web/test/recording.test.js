// Tests for the record + reverse-replay engine.
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { createRecorder } from '../js/recording.js';

let dispatch, motor, onStateChange, recorder;

beforeEach(() => {
  vi.useFakeTimers();
  // recording.js uses performance.now() for press timing — vitest's
  // fake-timers don't swap it in jsdom, so route it to the (faked)
  // Date.now() ourselves. Without this, test-time press durations are
  // real-clock (≈0 ms) and get filtered out as sub-100 ms mis-clicks.
  vi.spyOn(performance, 'now').mockImplementation(() => Date.now());
  dispatch = vi.fn();
  motor = vi.fn();
  onStateChange = vi.fn();
  recorder = createRecorder({ dispatch, motor, onStateChange });
});
afterEach(() => {
  vi.useRealTimers();
  vi.restoreAllMocks();
});

describe('createRecorder', () => {
  it('starts with empty state', () => {
    expect(recorder.isRecording()).toBe(false);
    expect(recorder.isReplaying()).toBe(false);
    expect(recorder.moveCount()).toBe(0);
  });

  it('startRecording flips state and notifies', () => {
    recorder.startRecording();
    expect(recorder.isRecording()).toBe(true);
    expect(onStateChange).toHaveBeenLastCalledWith({
      recording: true, replaying: false, moveCount: 0,
    });
  });

  it('press+release while recording captures duration', () => {
    recorder.startRecording();
    recorder.onPress('forward');
    vi.advanceTimersByTime(500);
    recorder.onRelease('forward');
    recorder.stopRecording();
    expect(recorder.moveCount()).toBe(1);
  });

  it('drops sub-100ms taps as mis-clicks', () => {
    recorder.startRecording();
    recorder.onPress('forward');
    vi.advanceTimersByTime(50);
    recorder.onRelease('forward');
    expect(recorder.moveCount()).toBe(0);
  });

  it('ignores `stop` action presses (only directional moves recorded)', () => {
    recorder.startRecording();
    recorder.onPress('stop');
    vi.advanceTimersByTime(500);
    recorder.onRelease('stop');
    expect(recorder.moveCount()).toBe(0);
  });

  it('release of unmatched action is ignored', () => {
    recorder.startRecording();
    recorder.onPress('forward');
    vi.advanceTimersByTime(200);
    recorder.onRelease('left');   // mismatched — should not commit
    recorder.onRelease('forward');
    expect(recorder.moveCount()).toBe(1);
  });

  it('stopRecording finalizes an in-flight press', () => {
    recorder.startRecording();
    recorder.onPress('left');
    vi.advanceTimersByTime(300);
    recorder.stopRecording();   // pressed but never released
    expect(recorder.moveCount()).toBe(1);
  });

  it('clearRecording wipes moves and resets recording flag', () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(200); recorder.onRelease('forward');
    recorder.stopRecording();
    expect(recorder.moveCount()).toBe(1);
    recorder.clearRecording();
    expect(recorder.moveCount()).toBe(0);
    expect(recorder.isRecording()).toBe(false);
  });

  it('replay walks the list backwards with each direction inverted', async () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(200); recorder.onRelease('forward');
    recorder.onPress('left');    vi.advanceTimersByTime(150); recorder.onRelease('left');
    recorder.onPress('forward'); vi.advanceTimersByTime(300); recorder.onRelease('forward');
    recorder.stopRecording();

    // Don't await — drive the timers.
    const done = recorder.startReplay();
    // First move replayed = last recorded reversed: forward (300ms) → backward.
    await vi.advanceTimersByTimeAsync(0);
    expect(dispatch).toHaveBeenLastCalledWith('backward');

    await vi.runAllTimersAsync();
    await done;

    // dispatch sequence should have been: backward, right, backward
    // (last move first, all directions flipped).
    const dispatched = dispatch.mock.calls.map(c => c[0]);
    expect(dispatched.filter(d => d !== 'stop')).toEqual([
      'backward', 'right', 'backward',
    ]);
  });

  it('replay calls motor(2,2) at the end (final stop)', async () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(200); recorder.onRelease('forward');
    recorder.stopRecording();

    const done = recorder.startReplay();
    await vi.runAllTimersAsync();
    await done;
    expect(motor).toHaveBeenCalledWith(2, 2);
  });

  it('startReplay refuses when already replaying or recording', async () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(200); recorder.onRelease('forward');
    expect(await recorder.startReplay()).toBeUndefined();
    recorder.stopRecording();
  });

  it('startReplay is a no-op with empty move list', async () => {
    expect(await recorder.startReplay()).toBeUndefined();
  });

  it('abortReplay stops mid-sequence', async () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(1500); recorder.onRelease('forward');
    recorder.onPress('left');    vi.advanceTimersByTime(1500); recorder.onRelease('left');
    recorder.stopRecording();

    const done = recorder.startReplay();
    await vi.advanceTimersByTimeAsync(50);   // we're mid-first-move
    recorder.abortReplay();
    await vi.runAllTimersAsync();
    const completed = await done;
    expect(completed).toBe(false);
  });

  it('completed replay returns true', async () => {
    recorder.startRecording();
    recorder.onPress('forward'); vi.advanceTimersByTime(200); recorder.onRelease('forward');
    recorder.stopRecording();

    const done = recorder.startReplay();
    await vi.runAllTimersAsync();
    expect(await done).toBe(true);
  });
});
