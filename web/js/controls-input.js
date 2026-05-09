// D-pad + keyboard input bindings for the control page.
//
// Tracks "currently held" buttons so spurious pointerleave events from a
// hover-without-click don't fire phantom releases that send STOP commands.

export function bindControls({ dispatchFn, onPress, onRelease }) {
  const buttons = document.querySelectorAll('.dpad button');
  const held = new Set();
  let keepaliveTimer = null;
  let heldAction = null;

  function startKeepalive(act) {
    heldAction = act;
    clearInterval(keepaliveTimer);
    // Re-issue the latest commands so the firmware's deadman (1500 ms)
    // sees fresh activity while the user holds a button. 'refresh'
    // kind bypasses control.js's dedup cache — without that, a steady
    // hold would only send commands once and the rover would stop.
    keepaliveTimer = setInterval(() => { if (heldAction) dispatchFn(heldAction, 'refresh'); }, 700);
  }
  function stopKeepalive() {
    heldAction = null;
    clearInterval(keepaliveTimer);
    keepaliveTimer = null;
  }

  function press(btn, act) {
    if (held.has(act)) return;
    held.add(act);
    btn.classList.add('active');
    onPress?.(act);
    dispatchFn(act, 'press');
    if (act !== 'stop') startKeepalive(act);
  }

  function release(btn, act) {
    if (!held.has(act)) return;
    held.delete(act);
    btn.classList.remove('active');
    onRelease?.(act);
    if (act !== 'stop') {
      stopKeepalive();
      // Tell the state machine THIS specific input was released — not
      // the same as 'stop' (which would clear all held inputs and kill
      // any other key the user is still holding).
      dispatchFn(act, 'release');
    }
  }

  buttons.forEach(btn => {
    const act = btn.dataset.act;
    btn.addEventListener('pointerdown', e => { e.preventDefault(); press(btn, act); });
    btn.addEventListener('pointerup',     e => { e.preventDefault(); release(btn, act); });
    btn.addEventListener('pointerleave',  e => { e.preventDefault(); release(btn, act); });
    btn.addEventListener('pointercancel', e => { e.preventDefault(); release(btn, act); });
  });

  // Keyboard
  const keyMap = { ArrowUp: 'forward', ArrowDown: 'backward', ArrowLeft: 'left', ArrowRight: 'right', ' ': 'stop' };
  const cssClass = { forward: 'up', backward: 'down', left: 'left', right: 'right', stop: 'stop' };
  const pressed = new Set();

  document.addEventListener('keydown', e => {
    const act = keyMap[e.key];
    if (!act || pressed.has(e.key)) return;
    pressed.add(e.key);
    e.preventDefault();
    const btn = document.querySelector('.dpad .' + cssClass[act]);
    if (btn) press(btn, act);
  });
  document.addEventListener('keyup', e => {
    const act = keyMap[e.key];
    if (!act) return;
    pressed.delete(e.key);
    const btn = document.querySelector('.dpad .' + cssClass[act]);
    if (btn) release(btn, act);
  });

  // Releasing focus → release any held button so motors stop.
  window.addEventListener('blur', () => {
    held.forEach(act => {
      const btn = document.querySelector('.dpad .' + cssClass[act]);
      if (btn) release(btn, act);
    });
  });
}
