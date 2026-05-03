// Tiny toast notification helper.
//
// Inserts a single .toast element on first use, then reuses it for subsequent
// calls. Pages just need to import { toast } and call toast('message', kind?).

let el = null;
let timer = null;

function ensureEl() {
  if (el) return el;
  el = document.createElement('div');
  el.className = 'toast';
  document.body.appendChild(el);
  return el;
}

export function toast(message, kind = '') {
  const node = ensureEl();
  node.textContent = message;
  node.className = 'toast show ' + kind;
  clearTimeout(timer);
  timer = setTimeout(() => { node.className = 'toast'; }, 2400);
}
