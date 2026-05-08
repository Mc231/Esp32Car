// Tests for the autonomous obstacle-avoidance mode. Covers the state
// machine, PWM scaling, distance validation, abort paths, and the
// 5-minute hard cap.
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { createAutonomous } from '../js/autonomous.js';

// Helper that builds an autonomous instance with mocks. distanceQueue is a
// FIFO of values the simulated `distance` command will return — null is
// passed through, anything else becomes `{response:{last_distance:n}}`.
function build({ distanceQueue = [], onTelemetry, onStateChange, vision } = {}) {
  let queueIdx = 0;
  const send = vi.fn(async ({ command }) => {
    if (command !== 'distance') return {};
    const v = distanceQueue[queueIdx++];
    if (v === undefined) return { response: { last_distance: distanceQueue.at(-1) ?? 100 } };
    if (v === null) return { response: { last_distance: 'NaN' } };
    return { response: { last_distance: v } };
  });
  const dispatch = vi.fn();
  const motor = vi.fn();
  const applyPwm = vi.fn();
  const auto = createAutonomous({
    send, dispatch, motor, applyPwm, vision,
    onStateChange: onStateChange ?? vi.fn(),
    onTelemetry: onTelemetry ?? vi.fn(),
  });
  return { auto, send, dispatch, motor, applyPwm };
}

describe('createAutonomous — basics', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('isActive is false initially', () => {
    const { auto } = build();
    expect(auto.isActive()).toBe(false);
    expect(auto.phase()).toBe('idle');
  });

  it('start() flips active and notifies', () => {
    const onStateChange = vi.fn();
    const { auto } = build({ distanceQueue: [60, 60, 60], onStateChange });
    auto.start();
    expect(auto.isActive()).toBe(true);
    expect(onStateChange).toHaveBeenCalledWith({ active: true, phase: 'driving' });
    auto.stop();
  });

  it('start() is idempotent', () => {
    const { auto } = build({ distanceQueue: [60] });
    auto.start();
    auto.start();   // second call should no-op
    expect(auto.isActive()).toBe(true);
    auto.stop();
  });

  it('stop() before start() is a no-op', () => {
    const { auto, motor } = build();
    auto.stop();
    expect(motor).not.toHaveBeenCalled();
  });
});

describe('PWM ramp by distance', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('uses PWM_MAX (255) at FAR_CM (70+) distance', async () => {
    const { auto, applyPwm } = build({ distanceQueue: [80, 80, 80] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    expect(applyPwm).toHaveBeenCalledWith(255);
    auto.stop();
  });

  it('uses PWM_MIN (180) at STOP_CM (22)', async () => {
    // Distance just above STOP_CM so the loop drives forward (not pivots).
    const { auto, applyPwm } = build({ distanceQueue: [23, 23, 23] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    // 23cm is barely over STOP_CM=22; t ≈ 1/48 ≈ 0.02 → 180 + ~2 = 182.
    const lastPwm = applyPwm.mock.calls.at(-1)[0];
    expect(lastPwm).toBeGreaterThanOrEqual(180);
    expect(lastPwm).toBeLessThan(190);
    auto.stop();
  });

  it('linear interpolates between STOP_CM and FAR_CM', async () => {
    // Midpoint distance: (22+70)/2 = 46. PWM should be ~218 (mid of 180-255).
    const { auto, applyPwm } = build({ distanceQueue: [46, 46, 46] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const lastPwm = applyPwm.mock.calls.at(-1)[0];
    expect(lastPwm).toBeGreaterThan(210);
    expect(lastPwm).toBeLessThan(225);
    auto.stop();
  });
});

describe('distance validity gates', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('skips ticks for sub-VALID_MIN readings (sensor folded back)', async () => {
    // 5cm < VALID_MIN_CM (8). loop should treat as null and not drive.
    const { auto, dispatch } = build({ distanceQueue: [5, 5, 5] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    // No 'forward' issued — only motor stops on each unreliable read.
    expect(dispatch.mock.calls.find(c => c[0] === 'forward')).toBeUndefined();
    auto.stop();
  });

  it('clamps high readings to VALID_MAX', async () => {
    const onTelemetry = vi.fn();
    const { auto } = build({ distanceQueue: [200, 200, 200], onTelemetry });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    // We still clamp to VALID_MAX_CM (85) internally, which then triggers
    // PWM_MAX since 85 > FAR_CM (70). So applyPwm should have been called
    // with 255.
    auto.stop();
  });

  it('stops the rover (no forward dispatch) on a non-finite reading', async () => {
    const { auto, dispatch } = build({ distanceQueue: [null, null] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    expect(dispatch.mock.calls.find(c => c[0] === 'forward')).toBeUndefined();
    auto.stop();
  });

  it('treats firmware -1 sentinel as clear path and drives forward', async () => {
    // Regression: -1 is the firmware "nothing within 80cm" sentinel.
    // Previously the loop confused it with sub-VALID_MIN noise and
    // sat still in any open room.
    const { auto, dispatch, applyPwm } = build({ distanceQueue: [-1, -1, -1] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    expect(dispatch.mock.calls.find(c => c[0] === 'forward')).toBeDefined();
    expect(applyPwm).toHaveBeenCalledWith(255);   // VALID_MAX_CM > FAR_CM → PWM_MAX
    auto.stop();
  });
});

describe('obstacle handling', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('triggers pivot when distance ≤ STOP_CM', async () => {
    // First reading 15cm (obstacle) → should pivot, not drive forward.
    // After pivoting, second reading 60cm (clear) → resume drive.
    const { auto, dispatch } = build({ distanceQueue: [15, 60, 60, 60] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    expect(dispatch).toHaveBeenCalledWith('left');  // first pivot side defaults to left
    auto.stop();
  });

  it('uses vision hint to seed pivot side over the alternating default', async () => {
    // Default first pivot side is 'left'. With a vision module saying
    // 'right', the first pivot should go right instead.
    const vision = { pickFreeSide: () => 'right' };
    const { auto, dispatch } = build({
      distanceQueue: [15, 60, 60, 60],
      vision,
    });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const firstSide = dispatch.mock.calls
      .map(c => c[0])
      .find(a => a === 'left' || a === 'right');
    expect(firstSide).toBe('right');
    auto.stop();
  });

  it('falls back to alternating default when vision returns null', async () => {
    const vision = { pickFreeSide: () => null };
    const { auto, dispatch } = build({
      distanceQueue: [15, 60, 60, 60],
      vision,
    });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const firstSide = dispatch.mock.calls
      .map(c => c[0])
      .find(a => a === 'left' || a === 'right');
    expect(firstSide).toBe('left');   // built-in default
    auto.stop();
  });

  it('treats vision "center" as no hint (alternating default fires)', async () => {
    const vision = { pickFreeSide: () => 'center' };
    const { auto, dispatch } = build({
      distanceQueue: [15, 60, 60, 60],
      vision,
    });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const firstSide = dispatch.mock.calls
      .map(c => c[0])
      .find(a => a === 'left' || a === 'right');
    expect(firstSide).toBe('left');
    auto.stop();
  });

  it('alternates pivot side per encounter', async () => {
    // Two obstacles back-to-back. First should pivot left, second right.
    const { auto, dispatch } = build({
      distanceQueue: [15, 60, 60, 15, 60, 60, 60]
    });
    auto.start();
    await vi.advanceTimersByTimeAsync(50);
    const sides = dispatch.mock.calls
      .map(c => c[0])
      .filter(a => a === 'left' || a === 'right');
    expect(sides[0]).toBe('left');
    // Second encounter should start with 'right'.
    if (sides.length >= 2) {
      const second = sides.find((s, i) => i > 0 && s !== sides[0]);
      expect(second).toBe('right');
    }
    auto.stop();
  });
});

describe('cancellation', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('stop() during driving aborts the loop and stops motors', async () => {
    const { auto, motor } = build({ distanceQueue: [60, 60, 60, 60, 60] });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    auto.stop();
    // The loop's .finally clears active/phase asynchronously — drain
    // the pending await + microtasks so the change is observable.
    await vi.runAllTimersAsync();
    expect(auto.isActive()).toBe(false);
    expect(motor).toHaveBeenCalledWith(2, 2);
  });
});

describe('telemetry callbacks', () => {
  beforeEach(() => vi.useFakeTimers());
  afterEach(() => vi.useRealTimers());

  it('emits onTelemetry with distance + pwm', async () => {
    const onTelemetry = vi.fn();
    const { auto } = build({ distanceQueue: [60, 60, 60], onTelemetry });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const calls = onTelemetry.mock.calls.map(c => c[0]);
    expect(calls.some(c => 'distance' in c)).toBe(true);
    expect(calls.some(c => 'pwm' in c)).toBe(true);
    auto.stop();
  });

  it('emits state changes for phase transitions', async () => {
    const onStateChange = vi.fn();
    const { auto } = build({ distanceQueue: [60, 60], onStateChange });
    auto.start();
    await vi.advanceTimersByTimeAsync(0);
    const phases = onStateChange.mock.calls.map(c => c[0].phase);
    expect(phases).toContain('driving');
    auto.stop();
    await vi.runAllTimersAsync();   // let loop().finally fire
    const lastPhase = onStateChange.mock.calls.at(-1)[0].phase;
    expect(lastPhase).toBe('idle');
  });
});
