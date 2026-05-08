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
const CLEAR_CM        = 30;        // resume driving once distance is over this
const IMPROVEMENT_CM  = 8;         // accept pivot if it improves clearance by this much
const HUG_CM          = 15;        // back up before pivoting when this close
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

export function createAutonomous({ send, dispatch, motor, applyPwm, onStateChange, onTelemetry, vision }) {
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
      // Firmware sentinel: -1.0 means "nothing within 80 cm". That's a
      // CLEAR path, not unreliable — drive at max. Without this the loop
      // pauses every tick in an open room and the rover sits still.
      if (d < 0) return VALID_MAX_CM;
      if (d < VALID_MIN_CM) return null;       // sensor fold-back / noise
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
  // `entryDistance` is the reading that triggered the pivot — used as a
  // baseline so a meaningful improvement counts as success even if the
  // ideal CLEAR_CM threshold isn't reached.
  async function findClearance(entryDistance) {
    // Seed pivot direction from vision if available — pick whichever
    // half of the lower frame looks LEAST cluttered. Falls back to the
    // alternating-side heuristic if vision returns null/center.
    const thirds      = vision?.getThirds?.();
    const freshnessMs = vision?.getFreshnessMs?.();
    const visionHint  = vision?.pickFreeSide?.();
    if (thirds) {
      console.log(
        `[auto] vision thirds: L=${thirds.left.toFixed(2)} ` +
        `C=${thirds.center.toFixed(2)} R=${thirds.right.toFixed(2)} ` +
        `(${freshnessMs != null ? freshnessMs.toFixed(0) + 'ms old' : 'fresh'}) ` +
        `→ hint=${visionHint ?? 'none'}`
      );
    } else if (vision) {
      console.log('[auto] vision: no frame analysed yet → falling back to alternating');
    }

    let side;
    if (visionHint === 'left' || visionHint === 'right') {
      side = visionHint;
    } else {
      side = state.nextPivotSide;
    }
    state.nextPivotSide = side === 'left' ? 'right' : 'left';

    // If we're hugging the wall, peel off first — pivoting in place
    // when scraping a wall just spins the wheels.
    if (entryDistance != null && entryDistance < HUG_CM) {
      await backup();
      if (state.abort) return false;
    }

    const startedAt = performance.now();
    let attempted = side;
    let flipped = false;
    let bestSeen = entryDistance ?? 0;

    while (!state.abort) {
      await pivotBurst(attempted);
      if (state.abort) return false;
      const d = await readDistance();
      if (d != null) {
        if (d >= CLEAR_CM) return true;
        if (d > bestSeen) bestSeen = d;
      }
      const elapsed = performance.now() - startedAt;
      if (elapsed > MAX_PIVOT_MS) {
        if (flipped) {
          // Both sides exhausted. If we found a notably better direction
          // somewhere along the way, accept it — outer loop drives toward
          // the best heading and re-evaluates. Beats giving up and
          // marching back into the same wall.
          if (bestSeen >= (entryDistance ?? 0) + IMPROVEMENT_CM) return true;
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
    let nullStreak = 0;
    while (!state.abort) {
      if (performance.now() - state.startedAt > HARD_CAP_MS) {
        console.log('[auto] hard cap hit — stopping');
        break;
      }
      const d = await readDistance();
      if (state.abort) break;
      if (d == null) {
        nullStreak++;
        console.log(`[auto] tick: distance=null (streak ${nullStreak}) — pausing`);
        // Repeated nulls usually mean the sensor is dead or the WS dropped —
        // exit autonomous rather than spin forever.
        if (nullStreak >= 25) {       // ~5 s at 200 ms tick
          console.warn('[auto] giving up — sensor unreliable for 5+ s');
          break;
        }
        motor(2, 2);
        await sleep(TICK_MS);
        continue;
      }
      nullStreak = 0;
      if (d <= STOP_CM) {
        console.log(`[auto] tick: distance=${d.toFixed(1)} cm <= ${STOP_CM} → pivot`);
        motor(2, 2);
        const ok = await findClearance(d);
        console.log(`[auto] findClearance → ${ok ? 'ok' : 'gave up'}`);
        if (state.abort || !ok) continue;
        continue;
      }
      console.log(`[auto] tick: distance=${d.toFixed(1)} cm → forward pwm=${pwmFor(d)}`);
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
