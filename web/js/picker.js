// Picker page — list of saved rovers, add/edit/remove, online status.

import { loadRovers, addRover, removeRover, renameRover } from './store.js';
import { probe } from './ws.js';
import { toast } from './toast.js';

const $ = id => document.getElementById(id);

function render() {
  const rovers = loadRovers();
  const list = $('roverList');
  const empty = $('emptyMsg');
  list.innerHTML = '';

  if (rovers.length === 0) {
    empty.hidden = false;
    return;
  }
  empty.hidden = true;

  rovers.forEach((r, idx) => {
    const row = document.createElement('div');
    row.className = 'rover';
    row.innerHTML = `
      <div class="dot warn" data-dot></div>
      <div class="info">
        <div class="name"></div>
        <div class="host"></div>
      </div>
      <div class="status warn" data-status>checking…</div>
      <div class="actions">
        <button class="icon-btn" data-edit aria-label="Rename">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12 20h9"/><path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4z"/>
          </svg>
        </button>
        <button class="icon-btn danger" data-del aria-label="Remove">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <polyline points="3 6 5 6 21 6"/>
            <path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/>
            <path d="M10 11v6M14 11v6"/>
            <path d="M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2"/>
          </svg>
        </button>
      </div>
    `;
    row.querySelector('.name').textContent = r.name;
    row.querySelector('.host').textContent = r.host;

    row.addEventListener('click', e => {
      if (e.target.closest('button')) return;
      location.href = `control.html?host=${encodeURIComponent(r.host)}&name=${encodeURIComponent(r.name)}`;
    });

    row.querySelector('[data-edit]').addEventListener('click', e => {
      e.stopPropagation();
      const newName = prompt('Rename rover:', r.name);
      if (newName && newName.trim()) {
        renameRover(idx, newName.trim());
        render();
      }
    });

    row.querySelector('[data-del]').addEventListener('click', e => {
      e.stopPropagation();
      if (!confirm(`Remove "${r.name}"?`)) return;
      removeRover(idx);
      render();
      toast('Removed');
    });

    list.appendChild(row);

    probe(r.host).then(online => {
      row.querySelector('[data-dot]').className = 'dot ' + (online ? 'ok' : 'err');
      const s = row.querySelector('[data-status]');
      s.className = 'status ' + (online ? 'ok' : 'err');
      s.textContent = online ? 'Online' : 'Offline';
    });
  });
}

$('addForm').addEventListener('submit', e => {
  e.preventDefault();
  const name = $('addName').value.trim();
  const host = $('addHost').value.trim();
  if (!name || !host) return;
  const result = addRover(name, host);
  if (!result.ok) {
    toast('A rover with that host already exists', 'error');
    return;
  }
  $('addName').value = '';
  $('addHost').value = '';
  render();
  toast(`Added "${name}"`, 'success');
});

render();
setInterval(render, 15000);   // refresh online status periodically
