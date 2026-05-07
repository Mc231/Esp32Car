// Autonomous obstacle-avoidance mode.
//
// Drives the rover forward, dynamically scaling PWM with the free distance
// ahead (Sharp GP2Y0A21, 10–80 cm). On obstacle, pivots in 180 ms bursts
// re-reading distance until clear, alternating initial side per encounter.
// Falls back to a brief reverse + flipped side after MAX_PIVOT_MS.
//
// All transport goes through the WS FIFO transport. The loop is strictly
// serial — at any given moment there's exactly one in-flight `send`.

const TICK_MS         = 200;
const STOP_CM         = 22;        // start pivoting at/under this
const CLEAR_CM        = 45;        // resume driving once distance is over this
const FAR_CM          = 70;        // PWM hits PWM_MAX at/over this
const PWM_MIN         = 180;
const PWM_MAX         = 255;
const PIVOT_PWM       = 180;
const PIVOT_BURST_MS  = 180;
const MAX_PIVOT_MS    = 3000;
const BACKUP_MS       = 350;
const HARD_CAP_MS     = 5 * 60 * 1000;
// GP2Y0A21 below ~10 cm folds back, above ~80 cm clamps. Treat both as suspect.
const VALID_MIN_CM    = 8;
const VALID_MAX_CM    = 85;

const sleep = ms => new Promise(r => setTimeout(r, ms));

export function createAutonomous({ send, dispatch, motor, applyPwm, onStateChange, onTelemetry }) {
  const state = {
    active: false,
    abort: false,
    phase: 'idle',          // idle | driving | pivoting | backup
    nextPivotSide: 'left',  // alternates each encounter
    startedAt: 0,
  };

  function notify() { onStateChange?.({ active: state.active, phase: state.phase }); }

  async function readDistance() {
    try {
      const msg = await send({ command: 'distance' });
      const d = parseFloat(msg?.response?.last_distance);
      if (!Number.isFinite(d)) return null;
      onTelemetry?.({ distance: d });
      // Out-of-range readings are reported as-is (we still need a usable
      // number for the loop). Treat them as "very far" so we keep moving;
      // anything inside [VALID_MIN_CM, VALID_MAX_CM] is trusted directly.
      if (d < VALID_MIN_CM) return null;       // unreliable — skip this tick
      if (d > VALID_MAX_CM) return VALID_MAX_CM;
      return d;
    } catch {
      return null;
    }
  }

  function pwmFor(distanceCm) {
    if (distanceCm >= FAR_CM) return PWM_MAX;
    if (distanceCm <= STOP_CM) return PWM_MIN;
    const t = (distanceCm - STOP_CM) / (FAR_CM - STOP_CM);
    return Math.round(PWM_MIN + t * (PWM_MAX - PWM_MIN));
  }

  // Drive forward at the PWM appropriate for `distanceCm`. Re-issuing the
  // command every tick keeps the rover's 1500 ms motor-deadman happy.
  async function driveForward(distanceCm) {
    state.phase = 'driving';
    notify();
    const pwm = pwmFor(distanceCm);
    onTelemetry?.({ pwm });
    applyPwm(pwm);
    dispatch('forward');
  }

  async function pivotBurst(side) {
    state.phase = 'pivoting';
    notify();
    applyPwm(PIVOT_PWM);
    onTelemetry?.({ pwm: PIVOT_PWM });
    dispatch(side);          // 'left' or 'right'
    await sleep(PIVOT_BURST_MS);
    if (state.abort) return;
    motor(2, 2);             // brief settle so the IR reading isn't taken mid-spin
    await sleep(60);
  }

  async function backup() {
    state.phase = 'backup';
    notify();
    applyPwm(PIVOT_PWM);
    dispatch('backward');
    await sleep(BACKUP_MS);
    motor(2, 2);
    await sleep(60);
  }

  // Pivot until clearance is found or we time out. Returns true on success.
  async function findClearance() {
    const side = state.nextPivotSide;
    state.nextPivotSide = side === 'left' ? 'right' : 'left';
    const startedAt = performance.now();
    let attempted = side;
    let flipped = false;
    while (!state.abort) {
      await pivotBurst(attempted);
      if (state.abort) return false;
      const d = await readDistance();
      if (d != null && d >= CLEAR_CM) return true;
      const elapsed = performance.now() - startedAt;
      if (elapsed > MAX_PIVOT_MS) {
        if (flipped) {
          // Already flipped once — give up this round, back up and let the
          // outer loop retry. Avoids a hopeless infinite spin.
          await backup();
          return false;
        }
        flipped = true;
        await backup();
        attempted = attempted === 'left' ? 'right' : 'left';
      }
    }
    return false;
  }

  async function loop() {
    state.startedAt = performance.now();
    while (!state.abort) {
      if (performance.now() - state.startedAt > HARD_CAP_MS) {
        // 5-minute safety cap.
        break;
      }
      const d = await readDistance();
      if (state.abort) break;
      if (d == null) {
        // Unreliable read — pause one tick rather than guess.
        motor(2, 2);
        await sleep(TICK_MS);
        continue;
      }
      if (d <= STOP_CM) {
        motor(2, 2);
        const ok = await findClearance();
        if (state.abort || !ok) continue;
        // Loop will re-evaluate; fall through to next iteration which reads
        // distance again before driving.
        continue;
      }
      await driveForward(d);
      await sleep(TICK_MS);
    }
    motor(2, 2);
  }

  function start() {
    if (state.active) return;
    state.active = true;
    state.abort = false;
    state.phase = 'driving';
    notify();
    loop().finally(() => {
      state.active = false;
      state.phase = 'idle';
      notify();
    });
  }

  function stop() {
    if (!state.active) return;
    state.abort = true;
    motor(2, 2);
  }

  return {
    start, stop,
    toggle: () => state.active ? stop() : start(),
    isActive: () => state.active,
    phase: () => state.phase,
  };
}
