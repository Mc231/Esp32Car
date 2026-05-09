// Pure logic for translating "currently-held inputs" into motor commands.
// Lives in its own module so the state machine can be unit-tested without
// dragging in the WS layer or the d-pad DOM bindings.
//
// Inputs come from the d-pad / keyboard:
//   - forward  / backward : linear direction
//   - left     / right    : turn (alone → tank pivot, with linear → arc)
//
// Output is a {leftAction, rightAction, leftPwm, rightPwm} record keyed
// to the firmware's wire enums:
//   action: 0 = FORWARD, 1 = BACKWARD, 2 = STOP
//   motor:  0 = left,    1 = right    (consumed by the caller, not here)

export const FORWARD  = 0;
export const BACKWARD = 1;
export const STOP     = 2;

// Inner-wheel PWM is this fraction of the slider speed during arc turns.
// Lower = sharper turn while moving. 0.4 felt right in bench testing —
// rover noticeably arcs but still makes forward progress.
export const ARC_INNER_RATIO = 0.4;

const STOP_BOTH = Object.freeze({
  leftAction: STOP, rightAction: STOP, leftPwm: 0, rightPwm: 0,
});

// `held`: any object/Set conveying which inputs are pressed. Accepts:
//   - { fwd, back, left, right } booleans
//   - a Set of strings 'forward' | 'backward' | 'left' | 'right'
// `baseSpeed`: the slider PWM (0..255).
export function computeMotorCommands(held, baseSpeed) {
  const { fwd, back, left, right } = normalise(held);
  const speed = clampPwm(baseSpeed);

  // Nothing held → both stop.
  if (!fwd && !back && !left && !right) return STOP_BOTH;

  // Conflicting linear keys → stop. Lets the user "kill" a stuck press
  // by tapping the opposite direction, matches keyboard expectations.
  if (fwd && back) return STOP_BOTH;

  // Both turn keys cancel each other out — same idea.
  const turnLeft  = left  && !right;
  const turnRight = right && !left;

  const linear = fwd ? FORWARD : (back ? BACKWARD : null);

  // Pure turn (no linear) → tank pivot in place. Inner wheel reverses
  // so the rover spins on its own axis.
  if (linear === null) {
    if (turnLeft)  return { leftAction: BACKWARD, rightAction: FORWARD, leftPwm: speed, rightPwm: speed };
    if (turnRight) return { leftAction: FORWARD,  rightAction: BACKWARD, leftPwm: speed, rightPwm: speed };
    return STOP_BOTH;   // both turns held / both cancelled
  }

  // Linear motion with optional arc.
  // Inner wheel = the side we're turning toward; reduce its PWM so the
  // rover curves without losing forward momentum entirely.
  const innerPwm = Math.round(speed * ARC_INNER_RATIO);
  const leftPwm  = turnLeft  ? innerPwm : speed;
  const rightPwm = turnRight ? innerPwm : speed;
  return { leftAction: linear, rightAction: linear, leftPwm, rightPwm };
}

function normalise(held) {
  if (!held) return { fwd: false, back: false, left: false, right: false };
  if (typeof held.has === 'function') {
    return {
      fwd:   held.has('forward'),
      back:  held.has('backward'),
      left:  held.has('left'),
      right: held.has('right'),
    };
  }
  return {
    fwd:   !!held.fwd,
    back:  !!held.back,
    left:  !!held.left,
    right: !!held.right,
  };
}

function clampPwm(v) {
  const n = parseInt(v, 10);
  if (!Number.isFinite(n)) return 0;
  if (n < 0) return 0;
  if (n > 255) return 255;
  return n;
}
