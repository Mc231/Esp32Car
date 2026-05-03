// Move recorder + reverse-replay engine.
//
// Records {action, durationMs} entries when the user presses & holds a
// d-pad button. Replay walks the list backwards with each direction
// inverted (forward<->backward, left<->right), pumping a keepalive every
// 700ms so the rover's deadman doesn't auto-stop us mid-move.

const REVERSE = { forward: 'backward', backward: 'forward', left: 'right', right: 'left' };
const reverseAction = act => REVERSE[act] || act;
const sleep = ms => new Promise(r => setTimeout(r, ms));

export function createRecorder({ dispatch, motor, onStateChange }) {
  const state = {
    recording: false,
    moves: [],
    replaying: false,
    abortReplay: false,
    pressStart: 0,
    pressAction: null,
  };

  function notify() { onStateChange?.(snapshot()); }

  function snapshot() {
    return {
      recording: state.recording,
      replaying: state.replaying,
      moveCount: state.moves.length,
    };
  }

  function startRecording() {
    if (state.replaying) return;
    state.recording = true;
    state.moves = [];
    state.pressStart = 0;
    state.pressAction = null;
    notify();
  }

  function stopRecording() {
    state.recording = false;
    // Finalise any in-flight press at the moment Stop is hit.
    if (state.pressAction && state.pressStart) {
      const duration = Math.round(performance.now() - state.pressStart);
      if (duration >= 100) state.moves.push({ action: state.pressAction, duration });
      state.pressStart = 0;
      state.pressAction = null;
    }
    notify();
  }

  function clearRecording() {
    state.recording = false;
    state.moves = [];
    notify();
  }

  // Called by input layer at press time.
  function onPress(act) {
    if (state.replaying) abortReplay();
    if (state.recording && act !== 'stop') {
      state.pressStart = performance.now();
      state.pressAction = act;
    }
  }

  // Called by input layer at release time.
  function onRelease(act) {
    if (state.recording && state.pressAction === act && state.pressStart) {
      const duration = Math.round(performance.now() - state.pressStart);
      state.pressStart = 0;
      state.pressAction = null;
      // Sub-100ms taps look like mis-clicks; ignore.
      if (duration >= 100) {
        state.moves.push({ action: act, duration });
        notify();
      }
    }
  }

  function abortReplay() {
    if (!state.replaying) return;
    state.abortReplay = true;
    motor(2, 2);
  }

  async function executeMove(act, durationMs) {
    dispatch(act);
    let elapsed = 0;
    const TICK = 700;
    while (elapsed < durationMs && !state.abortReplay) {
      const wait = Math.min(TICK, durationMs - elapsed);
      await sleep(wait);
      elapsed += wait;
      if (elapsed < durationMs && !state.abortReplay) dispatch(act);
    }
    motor(2, 2);
  }

  async function startReplay() {
    if (state.replaying || state.recording || state.moves.length === 0) return;
    state.replaying = true;
    state.abortReplay = false;
    notify();
    const sequence = [...state.moves].reverse();
    for (const move of sequence) {
      if (state.abortReplay) break;
      await executeMove(reverseAction(move.action), move.duration);
      if (!state.abortReplay) await sleep(200);
    }
    motor(2, 2);
    state.replaying = false;
    notify();
    return !state.abortReplay;   // true if completed normally
  }

  return {
    startRecording, stopRecording, clearRecording,
    startReplay, abortReplay,
    onPress, onRelease,
    snapshot,
    isRecording: () => state.recording,
    isReplaying: () => state.replaying,
    moveCount: () => state.moves.length,
  };
}
