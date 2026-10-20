// The whole web UI: one HTML file with inline CSS and JS, no external
// resources (the phone has no internet while connected to the AP).
// Served PROGMEM so no LittleFS data-folder upload step is needed.
#pragma once

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"WEBUI(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>Relay Timers</title>
<style>
:root {
  --bg: #0f1115;
  --card: #181b22;
  --card2: #1f232c;
  --text: #e6e8ee;
  --muted: #9aa1b2;
  --border: #2a2f3a;
  --accent: #24d4a0;
  --accent-text: #06201a;
  --force-on: #ffb020;
  --force-off: #ef5a6f;
  --danger: #ef5a6f;
}
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--text);
  font-family: -apple-system, system-ui, Segoe UI, Roboto, sans-serif; }
body { padding-bottom: 2rem; }
header { padding: 1rem 1rem 0 1rem; }
h1 { font-size: 1.2rem; margin: 0 0 0.75rem 0; text-align: center; }
h2 { font-size: 1rem; margin: 0 0 0.5rem 0; color: var(--muted); }
.tabs { display: flex; gap: 0.5rem; border-bottom: 1px solid var(--border); }
.tab-btn { flex: 1; padding: 0.75rem 0.5rem; background: none; border: none; color: var(--muted);
  font-size: 0.95rem; border-bottom: 3px solid transparent; }
.tab-btn.active { color: var(--text); border-bottom-color: var(--accent); }
main { padding: 1rem; max-width: 760px; margin: 0 auto; }
.tab { display: none; }
.tab.active { display: block; }
button { cursor: pointer; font-size: 0.95rem; border-radius: 10px; border: 1px solid var(--border);
  background: var(--card2); color: var(--text); padding: 0.6rem 0.9rem; min-height: 44px; }
button.primary { background: var(--accent); color: var(--accent-text); border-color: var(--accent); font-weight: 600; }
button.danger { background: var(--danger); color: #250b0e; border-color: var(--danger); }
button:disabled { opacity: 0.5; }
.banner { background: var(--force-on); color: #241804; padding: 0.75rem 1rem; border-radius: 10px;
  margin-bottom: 1rem; display: flex; align-items: center; justify-content: space-between; gap: 0.75rem;
  flex-wrap: wrap; }
.banner.hidden { display: none; }
.clock { color: var(--muted); margin-bottom: 1rem; font-size: 0.95rem; text-align: center;}
.cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 0.75rem; }
.card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 0.9rem; }
.ch-top { display: flex; align-items: center; justify-content: space-between; gap: 0.5rem; margin-bottom: 0.7rem; }
.ch-top .ch-label { flex: 1; min-width: 0; }
.ch-alias { display: block; font-weight: 600; }
.ch-channel { display: block; color: var(--text); font-weight: 600; }
.ch-channel.small { color: var(--muted); font-weight: 400; font-size: 0.8rem; }
.card .ch-state { flex: none; display: inline-block; padding: 0.15rem 0.6rem; border-radius: 999px; font-size: 0.8rem;
  background: var(--card2); color: var(--muted); }
.card .ch-state.on { background: var(--accent); color: var(--accent-text); }
.icon-btn { flex: none; background: none; border: none; min-height: auto; padding: 0.2rem 0.3rem;
  font-size: 1.15rem; line-height: 1; color: var(--muted); }
.icon-btn:active { color: var(--text); }
.mode-group { display: flex; gap: 1rem; }
.mode-group button { flex: 1; padding: 0.5rem 0.3rem; font-size: 0.85rem; }
.mode-group button.selected { border-color: var(--text); }
.mode-group button[data-mode="auto"].selected { background: var(--card2); border-color: var(--accent); color: var(--accent); }
.mode-group button[data-mode="on"].selected { background: var(--force-on); color: #241804; border-color: var(--force-on); }
.mode-group button[data-mode="off"].selected { background: var(--force-off); color: #250b0e; border-color: var(--force-off); }
#add-schedule-btn { margin-bottom: 1rem; width: 100%; }
.ch-group { margin-bottom: 1.25rem; }
.ch-group-title { margin-bottom: 0.4rem; }
.sched-row { background: var(--card); border: 1px solid var(--border); border-radius: 12px;
  padding: 0.7rem 0.9rem; margin-bottom: 0.5rem; display: flex; justify-content: space-between;
  align-items: center; gap: 0.5rem; }
.sched-row.disabled { opacity: 0.55; }
.sched-row .sched-main { flex: 1; min-width: 0; }
.sched-row .sched-name { font-weight: 600; }
.sched-row .sched-detail { color: var(--muted); font-size: 0.85rem; }
.empty-note { color: var(--muted); font-size: 0.9rem; }
.overlay { position: fixed; inset: 0; background: rgba(0,0,0,0.6); display: flex; align-items: flex-end;
  justify-content: center; z-index: 10; }
.overlay.hidden { display: none; }
.editor { background: var(--card); border-radius: 16px 16px 0 0; padding: 1rem; width: 100%;
  max-width: 480px; max-height: 92vh; overflow-y: auto; }
.editor label { display: block; margin-bottom: 0.75rem; font-size: 0.9rem; color: var(--muted); }
.editor input[type=text], .editor select { width: 100%; padding: 0.6rem; margin-top: 0.3rem;
  background: var(--card2); border: 1px solid var(--border); border-radius: 8px; color: var(--text);
  font-size: 1rem; min-height: 44px; }
.editor fieldset { border: 1px solid var(--border); border-radius: 10px; margin-bottom: 0.75rem; }
.editor legend { color: var(--muted); font-size: 0.85rem; padding: 0 0.4rem; }
.time-fields { display: flex; align-items: center; gap: 0.3rem; }
.time-fields input { width: 4rem; padding: 0.5rem; background: var(--card2); border: 1px solid var(--border);
  border-radius: 8px; color: var(--text); font-size: 1rem; text-align: center; min-height: 44px; }
.days-grid { display: flex; gap: 0.3rem; flex-wrap: wrap; }
.day-chip { padding: 0.5rem 0.7rem; border-radius: 999px; border: 1px solid var(--border);
  background: var(--card2); color: var(--muted); min-height: 44px; }
.day-chip.selected { background: var(--accent); color: var(--accent-text); border-color: var(--accent); }
.error { color: var(--force-off); font-size: 0.9rem; min-height: 1.2em; margin-bottom: 0.5rem; }
.editor-actions { display: flex; gap: 0.5rem; }
.editor-actions button { flex: 1; }
svg#week-chart { width: 100%; height: auto; background: var(--card); border-radius: 12px; }
.hidden { display: none; }
</style>
</head>
<body>
<header>
  <h1>Relay Timers</h1>
  <nav class="tabs">
    <button class="tab-btn active" data-tab="status">Status</button>
    <button class="tab-btn" data-tab="schedules">Schedules</button>
    <button class="tab-btn" data-tab="week">Week</button>
  </nav>
</header>
<main>
  <section id="tab-status" class="tab active">
    <div id="clock-banner" class="banner hidden">
      <span>Time not verified since last boot.</span>
      <button id="set-time-btn">Set time from this device</button>
    </div>
    <div id="clock-display" class="clock">&nbsp;</div>
    <div id="relay-cards" class="cards"></div>
  </section>

  <section id="tab-schedules" class="tab">
    <button id="add-schedule-btn" class="primary">+ New schedule</button>
    <div id="schedule-list"></div>
  </section>

  <section id="tab-week" class="tab">
    <svg id="week-chart" viewBox="0 0 690 320" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <pattern id="hatch" width="6" height="6" patternTransform="rotate(45)" patternUnits="userSpaceOnUse">
          <rect width="6" height="6" fill="none"></rect>
          <line x1="0" y1="0" x2="0" y2="6" stroke="var(--muted)" stroke-width="2"></line>
        </pattern>
      </defs>
    </svg>
  </section>
</main>

<div id="editor-overlay" class="overlay hidden">
  <div class="editor">
    <h2 id="editor-title">New schedule</h2>
    <div class="error" id="editor-error"></div>
    <label>Name
      <input type="text" id="f-name" maxlength="24" placeholder="Optional">
    </label>
    <label>Channel
      <select id="f-ch"></select>
    </label>
    <label>Days
      <div class="days-grid" id="f-days"></div>
    </label>
    <fieldset>
      <legend>Start time (hh:mm:ss)</legend>
      <div class="time-fields">
        <input type="number" id="f-start-h" min="0" max="23" value="0">:
        <input type="number" id="f-start-m" min="0" max="59" value="0">:
        <input type="number" id="f-start-s" min="0" max="59" value="0">
      </div>
    </fieldset>
    <fieldset>
      <legend>Duration (hh:mm:ss, max 24:00:00)</legend>
      <div class="time-fields">
        <input type="number" id="f-dur-h" min="0" max="24" value="0">:
        <input type="number" id="f-dur-m" min="0" max="59" value="30">:
        <input type="number" id="f-dur-s" min="0" max="59" value="0">
      </div>
    </fieldset>
    <label><input type="checkbox" id="f-en" checked> Enabled</label>
    <div class="editor-actions">
      <button id="save-btn" class="primary">Save</button>
      <button id="delete-btn" class="danger hidden">Delete</button>
      <button id="cancel-btn">Cancel</button>
    </div>
  </div>
</div>

<script>
'use strict';
const CHANNEL_COUNT = 8;
const DAY_NAMES = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
const SECONDS_PER_DAY = 86400;

let schedules = [];
let lastState = null;
let editingId = null; // null = creating

// ---- Channel friendly names ----------------------------------------------

function channelAlias(ch) {
  return (lastState && lastState.channels[ch] && lastState.channels[ch].name) || '';
}

// Renders the alias (if set) above a smaller "Channel N" line, or just
// "Channel N" at normal weight if no alias has been given.
function channelLabelHtml(alias, ch) {
  if (alias) {
    return '<span class="ch-alias">' + alias + '</span>' +
           '<span class="ch-channel small">Channel ' + (ch + 1) + '</span>';
  }
  return '<span class="ch-channel">Channel ' + (ch + 1) + '</span>';
}

async function renameChannel(ch) {
  const name = prompt('Friendly name for Channel ' + (ch + 1) + ' (leave blank to clear):', channelAlias(ch));
  if (name === null) return;
  if (name.length > 24) { alert('Name is too long (max 24 characters).'); return; }
  try {
    const state = await apiSend('/api/channels/' + ch + '/name', 'POST', { name });
    lastState = state;
    renderStatus(state);
    renderScheduleList();
    drawChart();
  } catch (e) {
    alert('Could not rename: ' + e.message);
  }
}

// ---- API helpers --------------------------------------------------------

async function apiGet(path) {
  const r = await fetch(path);
  if (!r.ok) throw new Error((await r.json().catch(() => ({}))).error || r.statusText);
  return r.json();
}
async function apiSend(path, method, body) {
  const r = await fetch(path, { method, headers: {'Content-Type':'application/json'}, body: JSON.stringify(body) });
  const data = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(data.error || r.statusText);
  return data;
}
async function apiDelete(path) {
  const r = await fetch(path, { method: 'DELETE' });
  if (!r.ok) {
    const data = await r.json().catch(() => ({}));
    throw new Error(data.error || r.statusText);
  }
}

// ---- Tabs ----------------------------------------------------------------

document.querySelectorAll('.tab-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    btn.classList.add('active');
    document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    if (btn.dataset.tab === 'week') drawChart();
  });
});

// ---- Status tab ------------------------------------------------------------

function fmtHMS(totalSeconds) {
  const h = Math.floor(totalSeconds / 3600);
  const m = Math.floor((totalSeconds % 3600) / 60);
  const s = totalSeconds % 60;
  return [h, m, s].map(n => String(n).padStart(2, '0')).join(':');
}

function renderStatus(state) {
  document.getElementById('clock-banner').classList.toggle('hidden', state.valid);
  document.getElementById('clock-display').textContent =
    'Board time: ' + (state.local || '--') + (state.valid ? '' : '  [unverified]');

  const cards = document.getElementById('relay-cards');
  cards.innerHTML = '';
  state.channels.forEach((c, ch) => {
    const card = document.createElement('div');
    card.className = 'card';
    card.innerHTML =
      '<div class="ch-top">' +
        '<div class="ch-label">' + channelLabelHtml(c.name, ch) + '</div>' +
        '<span class="ch-state' + (c.on ? ' on' : '') + '">' + (c.on ? 'ON' : 'OFF') + '</span>' +
        '<button type="button" class="icon-btn edit-btn" aria-label="Rename channel" title="Rename">&#9998;</button>' +
      '</div>' +
      '<div class="mode-group" data-ch="' + ch + '">' +
        '<button data-mode="auto">AUTO</button>' +
        '<button data-mode="on">ON</button>' +
        '<button data-mode="off">OFF</button>' +
      '</div>';
    card.querySelectorAll('.mode-group button').forEach(btn => {
      if (btn.dataset.mode === c.mode) btn.classList.add('selected');
      btn.addEventListener('click', () => setMode(ch, btn.dataset.mode));
    });
    card.querySelector('.edit-btn').addEventListener('click', () => renameChannel(ch));
    cards.appendChild(card);
  });
}

async function setMode(ch, mode) {
  try {
    const state = await apiSend('/api/relays/' + ch + '/mode', 'POST', { mode });
    lastState = state;
    renderStatus(state);
    drawChart();
  } catch (e) {
    alert('Could not set mode: ' + e.message);
  }
}

document.getElementById('set-time-btn').addEventListener('click', async () => {
  try {
    const epoch = Math.floor(Date.now() / 1000);
    const state = await apiSend('/api/time', 'POST', { epoch });
    lastState = state;
    renderStatus(state);
  } catch (e) {
    alert('Could not set time: ' + e.message);
  }
});

async function refreshState() {
  try {
    const state = await apiGet('/api/state');
    lastState = state;
    renderStatus(state);
    updateNowLine();
    updateOverrideBadges();
  } catch (e) { /* AP link hiccup; next poll retries */ }
}

// ---- Schedules tab -------------------------------------------------------

async function loadSchedules() {
  schedules = await apiGet('/api/schedules');
  renderScheduleList();
}

function renderScheduleList() {
  const list = document.getElementById('schedule-list');
  list.innerHTML = '';
  for (let ch = 0; ch < CHANNEL_COUNT; ch++) {
    const group = document.createElement('div');
    group.className = 'ch-group';
    const items = schedules.filter(s => s.ch === ch);
    group.innerHTML = '<div class="ch-group-title">' + channelLabelHtml(channelAlias(ch), ch) + '</div>';
    if (items.length === 0) {
      const note = document.createElement('div');
      note.className = 'empty-note';
      note.textContent = 'No schedules yet.';
      group.appendChild(note);
    } else {
      items.forEach(s => {
        const row = document.createElement('div');
        row.className = 'sched-row' + (s.en ? '' : ' disabled');
        const days = DAY_NAMES.filter((_, i) => s.days & (1 << i)).join(', ');
        row.innerHTML =
          '<div class="sched-main">' +
            '<div class="sched-name">' + (s.name || '(unnamed)') + '</div>' +
            '<div class="sched-detail">Start: ' + fmtHMS(s.start) + ', Duration: ' + fmtHMS(s.dur) + '</div>' +
            '<div class="sched-detail">' + days + '</div>' +
          '</div>';
        row.addEventListener('click', () => openEditor(s.id));
        group.appendChild(row);
      });
    }
    list.appendChild(group);
  }
}

document.getElementById('add-schedule-btn').addEventListener('click', () => openEditor(null));

// ---- Editor ----------------------------------------------------------------

function populateChannelSelect() {
  const sel = document.getElementById('f-ch');
  sel.innerHTML = '';
  for (let ch = 0; ch < CHANNEL_COUNT; ch++) {
    const opt = document.createElement('option');
    opt.value = ch;
    opt.textContent = 'Channel ' + (ch + 1);
    sel.appendChild(opt);
  }
}

function populateDayChips(selectedMask) {
  const container = document.getElementById('f-days');
  container.innerHTML = '';
  DAY_NAMES.forEach((name, i) => {
    const chip = document.createElement('button');
    chip.type = 'button';
    chip.className = 'day-chip' + ((selectedMask & (1 << i)) ? ' selected' : '');
    chip.textContent = name;
    chip.dataset.day = i;
    chip.addEventListener('click', () => chip.classList.toggle('selected'));
    container.appendChild(chip);
  });
}

function openEditor(id) {
  editingId = id;
  populateChannelSelect();
  document.getElementById('editor-error').textContent = '';

  const s = id === null ? null : schedules.find(x => x.id === id);
  document.getElementById('editor-title').textContent = s ? 'Edit schedule' : 'New schedule';
  document.getElementById('f-name').value = s ? s.name : '';
  document.getElementById('f-ch').value = s ? s.ch : 0;
  populateDayChips(s ? s.days : 0);

  const start = s ? s.start : 0;
  document.getElementById('f-start-h').value = Math.floor(start / 3600);
  document.getElementById('f-start-m').value = Math.floor((start % 3600) / 60);
  document.getElementById('f-start-s').value = start % 60;

  const dur = s ? s.dur : 1800;
  document.getElementById('f-dur-h').value = Math.floor(dur / 3600);
  document.getElementById('f-dur-m').value = Math.floor((dur % 3600) / 60);
  document.getElementById('f-dur-s').value = dur % 60;

  document.getElementById('f-en').checked = s ? !!s.en : true;
  document.getElementById('delete-btn').classList.toggle('hidden', !s);

  document.getElementById('editor-overlay').classList.remove('hidden');
}

function closeEditor() {
  document.getElementById('editor-overlay').classList.add('hidden');
  editingId = null;
}
document.getElementById('cancel-btn').addEventListener('click', closeEditor);

function readEditorForm() {
  const name = document.getElementById('f-name').value.trim();
  const ch = parseInt(document.getElementById('f-ch').value, 10);
  let days = 0;
  document.querySelectorAll('#f-days .day-chip.selected').forEach(chip => {
    days |= (1 << parseInt(chip.dataset.day, 10));
  });
  const startH = parseInt(document.getElementById('f-start-h').value || '0', 10);
  const startM = parseInt(document.getElementById('f-start-m').value || '0', 10);
  const startS = parseInt(document.getElementById('f-start-s').value || '0', 10);
  const durH = parseInt(document.getElementById('f-dur-h').value || '0', 10);
  const durM = parseInt(document.getElementById('f-dur-m').value || '0', 10);
  const durS = parseInt(document.getElementById('f-dur-s').value || '0', 10);
  const en = document.getElementById('f-en').checked;

  return {
    name, ch, days,
    start: startH * 3600 + startM * 60 + startS,
    dur: durH * 3600 + durM * 60 + durS,
    en,
  };
}

function validateForm(f) {
  if (f.name.length > 24) return 'Name is too long (max 24 characters).';
  if (f.ch < 0 || f.ch >= CHANNEL_COUNT) return 'Invalid channel.';
  if (f.days === 0) return 'Select at least one day.';
  if (f.start < 0 || f.start > 86399) return 'Start time must be within one day.';
  if (f.dur < 1 || f.dur > 86400) return 'Duration must be between 1 second and 24:00:00.';
  return null;
}

document.getElementById('save-btn').addEventListener('click', async () => {
  const form = readEditorForm();
  const err = validateForm(form);
  if (err) { document.getElementById('editor-error').textContent = err; return; }
  try {
    if (editingId === null) {
      await apiSend('/api/schedules', 'POST', form);
    } else {
      await apiSend('/api/schedules/' + editingId, 'PUT', form);
    }
    closeEditor();
    await loadSchedules();
    drawChart();
  } catch (e) {
    document.getElementById('editor-error').textContent = e.message;
  }
});

document.getElementById('delete-btn').addEventListener('click', async () => {
  if (editingId === null) return;
  if (!confirm('Delete this schedule?')) return;
  try {
    await apiDelete('/api/schedules/' + editingId);
    closeEditor();
    await loadSchedules();
    drawChart();
  } catch (e) {
    document.getElementById('editor-error').textContent = e.message;
  }
});

// ---- Weekly chart ----------------------------------------------------------
// Layout mirrors tests/cases.json's weekSecond convention (0 = Sunday
// 00:00:00) so this matches the firmware Evaluator exactly.

const CHART_LEFT = 70;
const CHART_TOP = 22;
const COL_WIDTH = (690 - CHART_LEFT) / 7;
const ROW_HEIGHT = (320 - CHART_TOP) / CHANNEL_COUNT;

function scheduleSegmentsForDay(s, day) {
  const segs = [];
  if (s.days & (1 << day)) {
    segs.push([s.start, Math.min(s.start + s.dur, SECONDS_PER_DAY)]);
  }
  const prevDay = (day + 6) % 7;
  if (s.days & (1 << prevDay)) {
    const end = s.start + s.dur;
    if (end > SECONDS_PER_DAY) {
      segs.push([0, Math.min(end - SECONDS_PER_DAY, SECONDS_PER_DAY)]);
    }
  }
  return segs;
}

function svgEl(tag, attrs) {
  const el = document.createElementNS('http://www.w3.org/2000/svg', tag);
  for (const k in attrs) el.setAttribute(k, attrs[k]);
  return el;
}

function drawChart() {
  const svg = document.getElementById('week-chart');
  while (svg.children.length > 1) svg.removeChild(svg.lastChild); // keep <defs>

  // Day headers
  for (let d = 0; d < 7; d++) {
    const x = CHART_LEFT + d * COL_WIDTH;
    svg.appendChild(svgEl('text', { x: x + COL_WIDTH / 2, y: 14, fill: 'var(--muted)',
      'font-size': 11, 'text-anchor': 'middle' }));
    svg.lastChild.textContent = DAY_NAMES[d];
  }

  // Channel labels + row separators + hour grid
  for (let ch = 0; ch < CHANNEL_COUNT; ch++) {
    const y = CHART_TOP + ch * ROW_HEIGHT;
    const alias = channelAlias(ch);
    if (alias) {
      svg.appendChild(svgEl('text', { x: 4, y: y + ROW_HEIGHT / 2 - 2, fill: 'var(--text)',
        'font-size': 10, 'font-weight': 600 }));
      svg.lastChild.textContent = alias;
      svg.appendChild(svgEl('text', { x: 4, y: y + ROW_HEIGHT / 2 + 10, fill: 'var(--muted)', 'font-size': 8 }));
      svg.lastChild.textContent = 'Channel ' + (ch + 1);
    } else {
      svg.appendChild(svgEl('text', { x: 4, y: y + ROW_HEIGHT / 2 + 4, fill: 'var(--muted)', 'font-size': 11 }));
      svg.lastChild.textContent = 'Ch ' + (ch + 1);
    }
    svg.appendChild(svgEl('line', { x1: CHART_LEFT, y1: y, x2: 690, y2: y, stroke: 'var(--border)' }));
  }
  svg.appendChild(svgEl('line', { x1: CHART_LEFT, y1: CHART_TOP + CHANNEL_COUNT * ROW_HEIGHT,
    x2: 690, y2: CHART_TOP + CHANNEL_COUNT * ROW_HEIGHT, stroke: 'var(--border)' }));

  for (let d = 0; d <= 7; d++) {
    const x = CHART_LEFT + d * COL_WIDTH;
    svg.appendChild(svgEl('line', { x1: x, y1: CHART_TOP, x2: x, y2: CHART_TOP + CHANNEL_COUNT * ROW_HEIGHT,
      stroke: 'var(--border)', 'stroke-width': 1.5 }));
    if (d < 7) {
      for (let h = 1; h < 24; h++) {
        const hx = x + (h / 24) * COL_WIDTH;
        svg.appendChild(svgEl('line', { x1: hx, y1: CHART_TOP, x2: hx, y2: CHART_TOP + CHANNEL_COUNT * ROW_HEIGHT,
          stroke: 'var(--border)', 'stroke-width': h % 6 === 0 ? 1 : 0.5, opacity: h % 6 === 0 ? 0.8 : 0.4 }));
      }
    }
  }

  // Bars
  const pad = 4;
  for (let ch = 0; ch < CHANNEL_COUNT; ch++) {
    const y = CHART_TOP + ch * ROW_HEIGHT + pad;
    const h = ROW_HEIGHT - 2 * pad;
    for (let d = 0; d < 7; d++) {
      const colX = CHART_LEFT + d * COL_WIDTH;
      schedules.filter(s => s.ch === ch).forEach(s => {
        scheduleSegmentsForDay(s, d).forEach(([segStart, segEnd]) => {
          const x = colX + (segStart / SECONDS_PER_DAY) * COL_WIDTH;
          const w = ((segEnd - segStart) / SECONDS_PER_DAY) * COL_WIDTH;
          const rect = svgEl('rect', { x, y, width: Math.max(w, 1), height: h, rx: 3,
            fill: s.en ? 'var(--accent)' : 'url(#hatch)',
            stroke: s.en ? 'none' : 'var(--muted)', 'stroke-width': s.en ? 0 : 1 });
          rect.style.cursor = 'pointer';
          rect.addEventListener('click', () => { switchTab('schedules'); openEditor(s.id); });
          svg.appendChild(rect);
        });
      });
    }
  }

  // Override badges (drawn after bars so they sit on top)
  svg.dataset.drawn = '1';
  updateOverrideBadges();
  updateNowLine();
}

function switchTab(name) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.toggle('active', b.dataset.tab === name));
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.id === 'tab-' + name));
}

function updateOverrideBadges() {
  const svg = document.getElementById('week-chart');
  svg.querySelectorAll('.override-badge').forEach(el => el.remove());
  if (!lastState) return;
  lastState.channels.forEach((c, ch) => {
    if (c.mode === 'auto') return;
    const y = CHART_TOP + ch * ROW_HEIGHT;
    const badge = svgEl('text', { x: 4, y: y + 11, fill: c.mode === 'on' ? 'var(--force-on)' : 'var(--force-off)',
      'font-size': 9, class: 'override-badge' });
    badge.textContent = c.mode === 'on' ? 'FORCED ON' : 'FORCED OFF';
    svg.appendChild(badge);
  });
}

function updateNowLine() {
  const svg = document.getElementById('week-chart');
  let line = svg.querySelector('#now-line');
  if (!lastState) return;
  const day = Math.floor(lastState.weekSecond / SECONDS_PER_DAY);
  const secOfDay = lastState.weekSecond % SECONDS_PER_DAY;
  const x = CHART_LEFT + day * COL_WIDTH + (secOfDay / SECONDS_PER_DAY) * COL_WIDTH;
  if (!line) {
    line = svgEl('line', { id: 'now-line', y1: CHART_TOP, y2: CHART_TOP + CHANNEL_COUNT * ROW_HEIGHT,
      stroke: 'var(--force-on)', 'stroke-width': 2 });
    svg.appendChild(line);
  }
  line.setAttribute('x1', x);
  line.setAttribute('x2', x);
}

// ---- Boot --------------------------------------------------------------------

(async function init() {
  await loadSchedules();
  await refreshState();
  drawChart();
  setInterval(refreshState, 2000);
})();
</script>
</body>
</html>
)WEBUI";
