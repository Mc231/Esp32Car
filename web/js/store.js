// localStorage-backed list of saved rovers.
// Each entry: { name: string, host: string }

const KEY = 'rovers.v1';

export function loadRovers() {
  try { return JSON.parse(localStorage.getItem(KEY)) || []; }
  catch { return []; }
}

export function saveRovers(rovers) {
  localStorage.setItem(KEY, JSON.stringify(rovers));
}

export function addRover(name, host) {
  const list = loadRovers();
  if (list.find(r => r.host === host)) return { ok: false, reason: 'exists' };
  list.push({ name, host });
  saveRovers(list);
  return { ok: true };
}

export function removeRover(idx) {
  const list = loadRovers();
  list.splice(idx, 1);
  saveRovers(list);
}

export function renameRover(idx, name) {
  const list = loadRovers();
  if (!list[idx]) return;
  list[idx] = { ...list[idx], name };
  saveRovers(list);
}
