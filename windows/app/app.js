/* KeoLive — tableau de bord des prochains passages en Île-de-France
   Données : PRIM (Île-de-France Mobilités), open data IDFM et SNCF, Train Nomade. */
'use strict';

// L'application : nom, version, propriétaire, entreprise (fenêtre « À propos »)
const APP = { name: 'KeoLive', version: '1.5.3', owner: 'Jean-François LAMBIN', company: 'Keolis' };

// =============================================================================
//  Outils
// =============================================================================
const SVGNS = 'http://www.w3.org/2000/svg';
const $ = (s, r = document) => r.querySelector(s);

function el(tag, props, ...kids) {
  const e = document.createElement(tag);
  if (props) for (const [k, v] of Object.entries(props)) {
    if (v == null || v === false) continue;
    if (k === 'class') e.className = v;
    else if (k === 'style') e.style.cssText = v;
    else if (k === 'text') e.textContent = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else e.setAttribute(k, v === true ? '' : v);
  }
  for (const k of kids.flat(3)) if (k != null && k !== false) e.append(k.nodeType ? k : document.createTextNode(String(k)));
  return e;
}

function icon(id, cls) {
  const s = document.createElementNS(SVGNS, 'svg');
  if (cls) s.setAttribute('class', cls);
  const u = document.createElementNS(SVGNS, 'use');
  u.setAttribute('href', '#' + id);
  s.append(u);
  return s;
}

const store = {
  get(k, d) {
    try { const v = localStorage.getItem('metro.' + k); return v == null ? d : JSON.parse(v); } catch { return d; }
  },
  set(k, v) { try { localStorage.setItem('metro.' + k, JSON.stringify(v)); } catch { /* stockage indisponible */ } },
};

const norm = s => (s || '').normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase()
  .replace(/[-'’.()/]/g, ' ').replace(/\s+/g, ' ').trim();
const debounce = (fn, ms) => { let t; return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); }; };
const sleep = ms => new Promise(r => setTimeout(r, ms));

let timeShift = 0;                       // mode démonstration uniquement
const now = () => Date.now() + timeShift;

// ---- Heure de Paris (quel que soit le fuseau de l'ordinateur) -------------------------
const TZ = 'Europe/Paris';
const fmtHM = new Intl.DateTimeFormat('fr-FR', { timeZone: TZ, hour: '2-digit', minute: '2-digit' });
const fmtDay = new Intl.DateTimeFormat('fr-FR', { timeZone: TZ, weekday: 'short', day: 'numeric', month: 'short' });
const fmtParts = new Intl.DateTimeFormat('en-GB', { timeZone: TZ, hourCycle: 'h23', year: 'numeric', month: '2-digit',
  day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' });
const hm = ms => fmtHM.format(new Date(ms));

function parisParts(ms) {
  const o = {};
  for (const p of fmtParts.formatToParts(new Date(ms))) o[p.type] = p.value;
  return { y: +o.year, mo: +o.month, d: +o.day, h: +o.hour % 24, m: +o.minute, s: +o.second };
}
function parisToMs(y, mo, d, secs) {      // heure « murale » de Paris -> instant
  const wall = Date.UTC(y, mo - 1, d) + secs * 1000;
  let t = wall;
  for (let i = 0; i < 2; i++) {
    const p = parisParts(t);
    t -= Date.UTC(p.y, p.mo - 1, p.d, p.h, p.m, p.s) - wall;
  }
  return t;
}
const parseCompact = s => s && s.length >= 15  // « 20261120T110000 » (heure de Paris)
  ? parisToMs(+s.slice(0, 4), +s.slice(4, 6), +s.slice(6, 8), +s.slice(9, 11) * 3600 + +s.slice(11, 13) * 60 + +s.slice(13, 15))
  : NaN;
const dayKey = ms => { const p = parisParts(ms); return `${p.y}-${String(p.mo).padStart(2, '0')}-${String(p.d).padStart(2, '0')}`; };

function whenText(begin, end) {
  const same = dayKey(begin) === dayKey(end);
  const day = ms => dayKey(ms) === dayKey(now()) ? '' : fmtDay.format(new Date(ms)) + ' ';
  if (!end || isNaN(end)) return 'Depuis ' + day(begin) + hm(begin);
  if (begin <= now()) return 'Jusqu’à ' + day(end) + hm(end);
  return same ? `${day(begin)}de ${hm(begin)} à ${hm(end)}` : `Du ${day(begin)}${hm(begin)} au ${day(end)}${hm(end)}`;
}

// =============================================================================
//  Réglages, quota, fenêtre native
// =============================================================================
const DEFAULTS = { apiKey: '', refresh: 60, budget: 1000, mainlineOnly: true, onTop: false, compact: false, compactIdx: 0, theme: 'auto', fullscreen: false, alerts: 'all', followed: [], sound: true };
const settings = Object.assign({}, DEFAULTS, store.get('settings', {}));
const saveSettings = () => store.set('settings', settings);

const quota = {
  data: store.get('quota', {}),
  used(kind) { return this.data.day === dayKey(now()) ? (this.data[kind] || 0) : 0; },
  add(kind) {
    const d = dayKey(now());
    if (this.data.day !== d) this.data = { day: d };
    this.data[kind] = (this.data[kind] || 0) + 1;
    store.set('quota', this.data);
  },
};

const NATIVE = typeof window.metroNative === 'function';
function native(cmd, arg) {
  if (!NATIVE) return Promise.resolve(null);
  return window.metroNative(cmd, arg).catch(() => null);
}
function openLink(url) {
  if (NATIVE) native('open', url);
  else window.open(url, '_blank', 'noopener');
}

function toast(msg, ms = 2600) {
  const s = $('#status');
  s.textContent = msg;
  s.classList.add('show');
  clearTimeout(toast.t);
  toast.t = setTimeout(() => s.classList.remove('show'), ms);
}

// =============================================================================
//  Lignes : nom, mode, couleurs
// =============================================================================
const MODE_LABEL = { metro: 'Métro', rer: 'RER', tram: 'Tram', train: 'Train', bus: 'Bus', navette: 'Navette' };
const MODE_ICON = { metro: 'm-metro', rer: 'm-rer', tram: 'm-tram', train: 'm-train', bus: 'm-bus', navette: 'm-metro' };
const LINE_ORDER = Object.keys(LINES);
const lineCache = store.get('lines', {});    // lignes de bus (référentiel IDFM), gardées d'une fois sur l'autre

const lineIdOf = ref => { const m = /\b(C\d{5}|M1[67])\b/.exec(ref || ''); return m ? m[1] : ''; };
const lineInfo = id => LINES[id] || lineCache[id] || null;
const lineRank = id => { const i = LINE_ORDER.indexOf(id); return i < 0 ? 999 : i; };

function badge(id, size = '', fallback = '') {
  const li = lineInfo(id);
  if (!li) wantLine(id);
  const mode = li ? li.m : 'bus';
  const b = el('span', { class: `lb ${mode} ${size}`, title: li ? `${MODE_LABEL[mode] || ''} ${li.n}` : (fallback || 'Ligne') });
  b.textContent = li ? li.n : (fallback || '·');
  b.style.background = li ? li.c : '#36405a';
  b.style.color = li ? li.t : '#fff';
  return b;
}

const ODS_IDFM = 'https://data.iledefrance-mobilites.fr/api/explore/v2.1/catalog/datasets/';
const pendingLines = new Set();
const missingLines = new Set();
const loadLinesSoon = debounce(loadLines, 400);
function wantLine(id) {
  if (!/^C\d{5}$/.test(id) || lineInfo(id) || missingLines.has(id)) return;
  pendingLines.add(id);
  loadLinesSoon();
}
async function loadLines() {
  const ids = [...pendingLines].slice(0, 30);
  ids.forEach(i => { pendingLines.delete(i); missingLines.add(i); });
  if (!ids.length) return;
  const where = ids.map(i => `id_line="${i}"`).join(' or ');
  const url = ODS_IDFM + 'referentiel-des-lignes/records?limit=30&select=id_line,shortname_line,transportmode,' +
    'colourweb_hexa,textcolourweb_hexa&where=' + encodeURIComponent(where);
  try {
    const j = await (await fetch(url)).json();
    const modes = { metro: 'metro', rail: 'rer', tram: 'tram', bus: 'bus', funicular: 'navette', cableway: 'navette' };
    for (const r of j.results || []) {
      lineCache[r.id_line] = { n: r.shortname_line || '?', m: modes[r.transportmode] || 'bus',
        c: '#' + (r.colourweb_hexa || '36405a'), t: '#' + (r.textcolourweb_hexa || 'ffffff') };
      missingLines.delete(r.id_line);
    }
    store.set('lines', lineCache);
    renderAll();
  } catch { /* hors ligne : pictogrammes neutres */ }
  if (pendingLines.size) loadLinesSoon();
}

// =============================================================================
//  API PRIM (Île-de-France Mobilités)
// =============================================================================
const PRIM = 'https://prim.iledefrance-mobilites.fr/marketplace';

class ApiError extends Error {
  constructor(kind, msg) { super(msg); this.kind = kind; }
}

async function primGet(path, kind = 'prim') {
  if (!settings.apiKey) throw new ApiError('nokey', 'Clé PRIM manquante');
  if (kind === 'prim' && quota.used('prim') >= settings.budget) throw new ApiError('quota', 'Quota du jour atteint');
  let r;
  try {
    r = await fetch(PRIM + path, { headers: { apikey: settings.apiKey, Accept: 'application/json' } });
  } catch {
    throw new ApiError('net', 'PRIM injoignable (réseau ou pare-feu)');
  }
  quota.add(kind);
  if (r.status === 401 || r.status === 403) throw new ApiError('key', `Clé PRIM refusée (${r.status})`);
  if (r.status === 429) throw new ApiError('quota', 'Quota PRIM dépassé (429)');
  if (r.status === 400) throw new ApiError('http', 'Arrêt inconnu de PRIM');
  if (!r.ok) throw new ApiError('http', `PRIM indisponible (${r.status})`);
  try { return await r.json(); } catch { throw new ApiError('http', 'Réponse PRIM illisible'); }
}

const val = a => Array.isArray(a) ? (a[0] && a[0].value) || '' : (a && a.value) || '';

function parseVisits(json) {
  const del = (((json || {}).Siri || {}).ServiceDelivery || {}).StopMonitoringDelivery || [];
  const out = [];
  for (const d of del) for (const v of d.MonitoredStopVisit || []) {
    const j = v.MonitoredVehicleJourney || {}, c = j.MonitoredCall || {};
    const line = lineIdOf(val(j.LineRef) || (j.LineRef && j.LineRef.value));
    const t = Date.parse(c.ExpectedDepartureTime || c.ExpectedArrivalTime || c.AimedDepartureTime || c.AimedArrivalTime || '');
    if (!line || isNaN(t)) continue;
    const st = String(c.DepartureStatus || c.ArrivalStatus || '').toLowerCase();
    let plat = val(c.DeparturePlatformName) || val(c.ArrivalPlatformName);
    if (/unknown|^-?$/i.test(plat)) plat = '';
    out.push({
      id: v.ItemIdentifier || `${line}-${t}`, line, t,
      aimed: Date.parse(c.AimedDepartureTime || c.AimedArrivalTime || ''),
      dest: val(c.DestinationDisplay) || val(j.DestinationName) || '—',
      cancelled: st === 'cancelled', delayed: st === 'delayed', atStop: c.VehicleAtStop === true,
      plat, mission: val(j.JourneyNote), name: val(j.PublishedLineName),
    });
  }
  return out;
}

// ---- Info trafic (toutes les lignes ferrées : métro, RER, Transilien, tram) ---------------
const traffic = { items: [], status: {}, fetchedAt: 0, err: null, loading: false };

function htmlText(s) {
  if (!s) return '';
  const src = String(s).replace(/<br\s*\/?>/gi, '\n').replace(/<\/(p|li|div)>/gi, '\n');
  const doc = new DOMParser().parseFromString(src, 'text/html');   // jamais injecté : texte seulement
  return (doc.body.textContent || '').replace(/[ \t]+\n/g, '\n').replace(/\n{3,}/g, '\n\n').trim();
}
const cleanTitle = t => htmlText(t).replace(/^(Métro|RER|Tram(way)?|Train|Ligne|Transilien)?\s*[\w-]{0,4}\s*:\s*/i, '');

function parseTraffic(json) {
  const dis = new Map((json.disruptions || []).map(d => [d.id, d]));
  const t = now(), items = [], seen = new Set();
  for (const L of json.lines || []) {
    const id = lineIdOf(L.id);
    const li = LINES[id];
    if (!li || !['metro', 'rer', 'tram', 'train'].includes(li.m)) continue;
    const ids = new Set(), stops = new Map();   // perturbation -> stations touchées (vide : toute la ligne)
    for (const o of L.impactedObjects || []) for (const x of o.disruptionIds || []) {
      ids.add(x);
      if (!stops.has(x)) stops.set(x, new Set());
      if (o.type === 'stop_point' || o.type === 'stop_area') stops.get(x).add(norm(o.name));
      else stops.get(x).add('*');
    }
    for (const did of ids) {
      const d = dis.get(did);
      if (!d || seen.has(id + did)) continue;
      seen.add(id + did);
      let active = false, begin = NaN, end = NaN, next = Infinity;
      for (const p of d.applicationPeriods || []) {
        const b = parseCompact(p.begin), e = parseCompact(p.end);
        if (b <= t && t <= e) { active = true; begin = b; end = e; break; }
        if (b > t && b < next) { next = b; begin = b; end = e; }
      }
      if (!active && !(next - t < 24 * 3600e3)) continue;
      items.push({
        line: id, id: did, active, begin, end,
        level: d.severity === 'BLOQUANTE' ? 3 : d.severity === 'PERTURBEE' ? 2 : 1,
        works: d.cause === 'TRAVAUX', title: cleanTitle(d.title), short: htmlText(d.shortMessage), msg: htmlText(d.message),
        stops: stops.get(did).has('*') ? [] : [...stops.get(did)],
      });
    }
  }
  items.sort((a, b) => (b.active - a.active) || (b.level - a.level) || lineRank(a.line) - lineRank(b.line));
  const status = {};
  for (const it of items) if (it.active) {
    const s = status[it.line] || (status[it.line] = { level: 0, works: false });
    s.level = Math.max(s.level, it.level);
    if (it.works) s.works = true;
  }
  traffic.items = items;
  traffic.status = status;
  traffic.updated = Date.parse(json.lastUpdatedDate || '') || now();
}

// Info trafic relue toutes les 2 minutes quand les alertes sont actives (5 minutes sinon),
// y compris fenêtre réduite : c'est ce qui permet de prévenir dès qu'un incident survient.
const trafficEvery = () => settings.alerts === 'off' ? 300e3 : 120e3;
async function loadTraffic(force) {
  if (!settings.apiKey || traffic.loading) return;
  if (document.hidden && settings.alerts === 'off') return;
  if (!force && now() - traffic.fetchedAt < trafficEvery()) return;
  traffic.loading = true;
  try {
    parseTraffic(await primGet('/disruptions_bulk/disruptions/v2', 'trafic'));
    traffic.err = null;
    checkAlerts();
  } catch (e) {
    traffic.err = e.message;
  }
  traffic.fetchedAt = now();
  traffic.loading = false;
  renderTraffic();
  cards.forEach(c => c.render());
}
// Alerte sur une carte : perturbation de la ligne entière, ou de cette station
// Ligne pas encore ouverte (Grand Paris Express) : année d'ouverture prévue, sinon 0
function futureYear(id) {
  const l = NETWORK.find(x => x.id === id);
  return l && l.year > parisParts(now()).y ? l.year : 0;
}
const lineAlert = (id, stopName) => traffic.items.find(it => it.active && it.line === id && it.level >= 2 &&
  (!it.stops.length || !stopName || it.stops.includes(norm(stopName))));

// =============================================================================
//  Trains SNCF : Train Nomade (sans clé, avec les voies)
// =============================================================================
const TN = 'https://api.trainnomade.fr/gare/';
const TRAIN_CAT = { highSpeedRail: 'TGV', interregionalRail: 'Intercités', regionalRail: 'TER', local: 'TER',
  suburbanRailway: 'Transilien', tramTrain: 'Tram-train', railShuttle: 'Navette', longDistance: 'Grande ligne' };
// Grandes gares de Paris : une carte, une gare par onglet
const PARIS_GARES = SNCF_STATIONS.slice(0, 7).map(([name, uics]) => ({ name, uics, short: name.replace(/^Gare (de l'|de |du )/, '') }));
const parisModel = () => ({ type: 'paris', key: 'paris', station: 0, arrivals: false });
const isTrainCard = m => m.type === 'gare' || m.type === 'paris';

const cleanStation = s => (s || '').replace(/\s+(RER|Transilien|Grandes Lignes|Surface|Souterrain)$/i, '').trim();

function parseTrainNomade(j, arrivals) {
  if (!j || !Array.isArray(j.passages)) throw new Error(j && j.erreur ? 'Gare inconnue' : 'Réponse inattendue');
  const [y, mo, d] = String(j.jour || '').split('-').map(Number);
  const noms = j.noms || {}, out = [], seen = new Set();
  for (const p of j.passages) {
    const tv = p[arrivals ? 'a' : 'd'];
    if (typeof tv !== 'number' || p.p) continue;
    if (settings.mainlineOnly && ['suburbanRailway', 'tramTrain', 'railShuttle'].includes(p.pr)) continue;
    const num = String(p.n || '').split(/[\/-]/)[0];
    if (num && seen.has(num)) continue;
    seen.add(num);
    const [jy, jm, jd] = p.j ? String(p.j).split('-').map(Number) : [y, mo, d];
    const base = parisToMs(jy, jm, jd, tv);
    const code = p[arrivals ? 'o' : 'de'];
    out.push({ base, real: base + (p[arrivals ? 'ra' : 'rd'] || 0) * 1000, num, cat: TRAIN_CAT[p.pr] || 'Train',
      place: cleanStation(noms[code] || code || ''), plat: p.v || '', realtime: !p.nm, cancelled: !!(p.an || p.x) });
  }
  return out.sort((a, b) => a.real - b.real);
}

// =============================================================================
//  Cartes : un arrêt (métro, RER, tram, bus) ou une gare SNCF
// =============================================================================
const cards = [];
let preview = null;

class Card {
  constructor(model, opts = {}) {
    this.m = model;
    this.preview = !!opts.preview;
    this.data = null;
    this.err = null;
    this.loading = false;
    this.fetchedAt = 0;
    this.cache = {};   // tableaux SNCF déjà reçus, par gare (changement d'onglet immédiat)
    this.el = el('article', { class: 'card' + (isTrainCard(model) ? ' wide' : ''), 'data-key': model.key });
    if (!this.preview) this.dragAndDrop();
    this.render();
  }

  get requests() { return this.m.type === 'stop' ? this.m.refs.length : 0; }

  interval() { return isTrainCard(this.m) ? 60 : stopInterval(); }

  // Gare affichée (carte d'une gare, ou onglet choisi des grandes gares de Paris)
  get gare() {
    if (this.m.type !== 'paris') return { name: this.m.name, uics: this.m.uics };
    return PARIS_GARES[Math.min(PARIS_GARES.length - 1, Math.max(0, this.m.station | 0))];
  }

  due() { return !this.loading && now() - this.fetchedAt >= this.interval() * 1000; }

  async refresh() {
    if (this.loading) return;
    this.loading = true;
    if (!this.data) this.render();
    const gare = this.m.type === 'paris' ? this.m.station : null;
    try {
      if (this.m.type === 'stop') {
        const all = [];
        for (const zda of this.m.refs) {
          const ref = `STIF:StopArea:SP:${zda}:`;
          all.push(...parseVisits(await primGet('/stop-monitoring?MonitoringRef=' + encodeURIComponent(ref))));
        }
        const uniq = new Map(all.map(v => [v.id, v]));
        this.data = [...uniq.values()].sort((a, b) => a.t - b.t);
      } else {
        const uics = this.gare.uics;
        let r;
        try { r = await fetch(TN + uics + '.json'); } catch { throw new ApiError('net', 'Train Nomade injoignable'); }
        if (!r.ok) throw new ApiError('http', `Train Nomade indisponible (${r.status})`);
        const raw = await r.json();
        this.cache[uics] = { raw, at: now() };
        if (uics === this.gare.uics) {   // sinon : un autre onglet a été choisi entre-temps
          this.raw = raw;
          this.data = parseTrainNomade(this.raw, this.m.arrivals);
        }
      }
      this.err = null;
    } catch (e) {
      this.err = e;
    }
    this.loading = false;
    if (gare !== null && gare !== this.m.station) return this.refresh();   // autre onglet entre-temps
    this.fetchedAt = now();
    this.render();
  }

  render() {
    const m = this.m;
    const lines = this.lineIds();
    const train = isTrainCard(m), name = train ? this.gare.name : m.name;
    const head = el('header', { class: 'card-head', draggable: this.preview ? null : 'true' },
      el('div', { class: 'card-title' },
        el('div', { class: 'mode' }, icon(train ? 'm-train' : MODE_ICON[this.mainMode()] || 'm-bus'),
          m.type === 'paris' ? 'Train · Grandes gares de Paris' : train ? 'Train · ' + (m.arrivals ? 'Arrivées' : 'Départs')
            : m.sub || MODE_LABEL[this.mainMode()] || 'Arrêt'),
        el('div', { class: 'card-name', title: name }, name)),
      m.type === 'stop' ? el('div', { class: 'lbs' }, lines.slice(0, 6).map(id => badge(id, 'lg'))) : null,
      train ? el('div', { class: 'seg', role: 'group' },
        el('button', { 'aria-pressed': String(!m.arrivals), onclick: () => this.setArrivals(false) }, 'Départs'),
        el('button', { 'aria-pressed': String(!!m.arrivals), onclick: () => this.setArrivals(true) }, 'Arrivées')) : null,
      el('div', { class: 'card-acts' },
        el('button', { class: 'ibtn', title: 'Actualiser', onclick: () => this.refresh() }, icon('i-swap')),
        el('button', { class: 'ibtn', title: 'Retirer du tableau', onclick: () => unpin(m.key) }, icon('i-close'))));

    const bar = el('div', { class: 'card-bar' },
      (lines.length ? lines : ['']).slice(0, 8).map(id => el('i', { style: `background:${(lineInfo(id) || {}).c || '#ffb400'}` })));
    if (train) bar.replaceChildren(el('i', { style: 'background:var(--amber)' }));
    const tabs = m.type === 'paris' ? el('nav', { class: 'gtabs', role: 'tablist' }, PARIS_GARES.map((g, i) =>
      el('button', { class: 'gtab', role: 'tab', 'aria-pressed': String(i === (m.station | 0)), title: g.name,
        onclick: () => this.setStation(i) }, g.short))) : null;

    const alerts = lines.map(id => lineAlert(id, m.name)).filter(Boolean);
    const alert = alerts.length ? el('div', { class: 'card-alert l' + Math.max(...alerts.map(a => a.level)),
      onclick: () => openLine(alerts[0].line) }, icon('i-warn'), badge(alerts[0].line, 'sm'),
    el('span', {}, alerts[0].short || alerts[0].title)) : null;

    const foot = el('footer', { class: 'card-foot' },
      el('span', { class: 'grow' + (this.err && this.data ? ' err' : '') },
        this.err && this.data ? '⚠ ' + this.err.message
          : this.fetchedAt ? 'Mis à jour à ' + hm(this.fetchedAt) : ''),
      train ? el('span', {}, 'Train Nomade · SNCF') : el('span', {}, 'PRIM · IDFM'));

    this.el.replaceChildren(...[bar, head, tabs, alert, el('div', { class: 'card-body' }, this.body()), foot].filter(Boolean));
  }

  mainMode() {
    const ids = this.lineIds();
    for (const mode of ['metro', 'rer', 'train', 'tram', 'bus']) if (ids.some(id => (lineInfo(id) || {}).m === mode)) return mode;
    return this.m.mode || 'bus';
  }

  lineIds() {
    if (this.m.type !== 'stop') return [];
    const ids = new Set(this.m.lines || []);
    if (this.data) for (const v of this.data) ids.add(v.line);
    return [...ids].sort((a, b) => lineRank(a) - lineRank(b));
  }

  setArrivals(arr) {   // départs et arrivées sont dans la même réponse
    if (!!this.m.arrivals === arr) return;
    this.m.arrivals = arr;
    if (!this.preview) savePins();
    this.data = this.raw ? parseTrainNomade(this.raw, arr) : null;
    if (!this.data) this.fetchedAt = 0;
    this.render();
  }

  setStation(i) {
    if ((this.m.station | 0) === i) return;
    this.m.station = i;
    if (!this.preview) savePins();
    const c = this.cache[this.gare.uics];
    if (c && now() - c.at < 60e3) {
      this.raw = c.raw;
      this.data = parseTrainNomade(c.raw, this.m.arrivals);
      this.fetchedAt = c.at;
      this.err = null;
    } else {
      this.raw = null;
      this.data = null;
      this.err = null;
      this.fetchedAt = 0;
      this.refresh();
    }
    this.render();
  }

  body() {
    if (this.m.type === 'stop' && !settings.apiKey)
      return el('div', { class: 'state' }, icon('i-key'), el('b', {}, 'Clé PRIM nécessaire'),
        el('span', {}, 'Ajoutez votre clé gratuite dans les réglages.'));
    if (!this.data) {
      if (this.err) return el('div', { class: 'state err' }, icon('i-warn'), el('b', {}, this.err.message),
        el('span', {}, this.err.kind === 'key' ? 'Vérifiez la clé dans les réglages.' : 'Nouvel essai bientôt.'));
      return el('div', { class: 'state' }, el('div', { class: 'loader' }), 'Chargement…');
    }
    return this.m.type === 'stop' ? this.stopRows() : this.trainRows();
  }

  stopRows() {
    const t0 = now() - 30e3;
    const visits = this.data.filter(v => v.t >= t0 || v.atStop);
    if (!visits.length) return el('div', { class: 'state' }, el('b', {}, 'Aucun passage annoncé'),
      el('span', {}, 'Service terminé ou pas encore commencé.'));
    const max = this.preview ? 14 : (document.body.classList.contains('compact') ? 6 : 8);
    const rows = [];
    const byLine = new Map();
    for (const v of visits) (byLine.get(v.line) || byLine.set(v.line, []).get(v.line)).push(v);
    const lines = [...byLine.keys()].sort((a, b) => lineRank(a) - lineRank(b) || byLine.get(a)[0].t - byLine.get(b)[0].t);
    const perLine = Math.max(3, Math.floor(max / lines.length));   // trains par ligne de RER ou Transilien
    for (const line of lines) {
      const list = byLine.get(line);
      const mode = (lineInfo(line) || {}).m || 'bus';
      if (mode === 'rer' || mode === 'train') {     // RER, Transilien : trains dans l'ordre, avec la mission
        for (const v of list.slice(0, perLine)) rows.push({ line, dest: v.dest, times: [v], v });
      } else {                                      // métro, tram, bus : par destination, deux passages
        const byDest = new Map();
        for (const v of list) (byDest.get(v.dest) || byDest.set(v.dest, []).get(v.dest)).push(v);
        for (const [dest, vs] of [...byDest].sort((a, b) => a[1][0].t - b[1][0].t)) rows.push({ line, dest, times: vs.slice(0, 2), v: vs[0] });
      }
    }
    const shown = rows.slice(0, max);
    return [
      shown.map(r => el('div', { class: 'row' }, badge(r.line, 'lg', r.v.name),
        el('div', { class: 'dest' },
          el('div', { class: 'dest-name', title: r.dest }, icon('i-arrow'), r.dest),
          (r.v.mission || r.v.plat) ? el('div', { class: 'dest-info' },
            r.v.mission ? el('span', { class: 'chip' }, r.v.mission) : null,
            r.v.plat ? el('span', { class: 'chip voie' }, 'Voie ' + r.v.plat) : null) : null),
        el('div', { class: 'times' }, r.times.map((v, i) => timeCell(v, i > 0))))),
      rows.length > max ? el('div', { class: 'more' }, `+ ${rows.length - max} autre${rows.length - max > 1 ? 's' : ''} départ${rows.length - max > 1 ? 's' : ''}`) : null,
    ];
  }

  trainRows() {
    const list = this.data.filter(t => t.real >= now() - 60e3);
    if (!list.length) return el('div', { class: 'state' }, el('b', {}, this.m.arrivals ? 'Aucune arrivée prévue' : 'Aucun départ prévu'),
      el('span', {}, settings.mainlineOnly ? 'Grandes lignes seulement (réglages).' : 'Dans les prochaines heures.'));
    const max = this.preview ? 14 : 8;
    return [
      el('div', { class: 'trow head' }, el('span', {}, 'Heure'), el('span', {}, this.m.arrivals ? 'Provenance' : 'Destination'),
        el('span', {}, 'Train'), el('span', {}, 'Voie'), el('span', { style: 'text-align:right' }, 'État')),
      list.slice(0, max).map(t => {
        const late = Math.round((t.real - t.base) / 60000);
        const st = t.cancelled ? ['cancel', 'Supprimé'] : !t.realtime ? ['plan', 'Prévu'] : late > 0 ? ['late', `+${late} min`] : ['ok', 'À l’heure'];
        return el('div', { class: 'trow' },
          el('div', { class: 'th' }, late > 0 && !t.cancelled ? [el('s', {}, hm(t.base)), hm(t.real)] : hm(t.base)),
          el('div', { class: 'tdest', title: t.place }, t.place),
          el('div', { class: 'ttrain' }, el('span', { class: 'chip' }, t.cat), t.num),
          el('div', { class: 'tvoie' + (t.plat ? '' : ' none') }, t.plat || '—'),
          el('div', { class: 'tstate ' + st[0] }, st[1]));
      }),
    ];
  }

  dragAndDrop() {
    this.el.addEventListener('dragstart', e => { dragKey = this.m.key; this.el.classList.add('drag'); e.dataTransfer.effectAllowed = 'move'; });
    this.el.addEventListener('dragend', () => { this.el.classList.remove('drag'); document.querySelectorAll('.card.over').forEach(c => c.classList.remove('over')); });
    this.el.addEventListener('dragover', e => { if (dragKey && dragKey !== this.m.key) { e.preventDefault(); this.el.classList.add('over'); } });
    this.el.addEventListener('dragleave', () => this.el.classList.remove('over'));
    this.el.addEventListener('drop', e => { e.preventDefault(); this.el.classList.remove('over'); movePin(dragKey, this.m.key); });
  }
}
let dragKey = null;

function timeCell(v, next) {
  const min = Math.floor((v.t - now()) / 60000);
  if (v.cancelled) return el('div', { class: 't cancel' }, 'SUPPRIMÉ');
  if (v.atStop && min <= 0) return el('div', { class: 't now' + (next ? ' next' : '') }, 'À QUAI');
  if (min <= 0) return el('div', { class: 't now' + (next ? ' next' : '') }, 'APPROCHE');
  if (min >= 60) return el('div', { class: 't clock' + (next ? ' next' : '') }, hm(v.t));
  return el('div', { class: 't' + (next ? ' next' : '') + (v.delayed ? ' late' : ''), title: 'Passage à ' + hm(v.t) },
    String(min), el('small', {}, 'min'));
}

// Rythme des appels PRIM : réglage, ralenti pour tenir le quota de la journée (jusqu'à 20 h)
function stopInterval() {
  const perCycle = cards.reduce((n, c) => n + c.requests, 0) + (preview ? preview.requests : 0) || 1;
  const left = settings.budget - quota.used('prim');
  if (left <= 0) return 3600;
  const p = parisParts(now());
  const sec = p.h * 3600 + p.m * 60 + p.s;
  const end = sec < 20 * 3600 ? 20 * 3600 : 24 * 3600;
  return Math.max(settings.refresh, Math.ceil((end - sec) * perCycle / left));
}

// =============================================================================
//  Tableau : cartes épinglées
// =============================================================================
let pins = store.get('pins', []);
const savePins = () => store.set('pins', pins);
const isPinned = key => pins.some(p => p.key === key);

function pin(model) {
  if (isPinned(model.key)) return;
  pins.push(JSON.parse(JSON.stringify(model)));
  savePins();
  syncCards();
  toast('Épinglé au tableau');
}
function unpin(key) {
  if (settings.alerts === 'mine') setTimeout(resetAlerts);
  pins = pins.filter(p => p.key !== key);
  savePins();
  syncCards();
}
function movePin(from, to) {
  const a = pins.findIndex(p => p.key === from), b = pins.findIndex(p => p.key === to);
  if (a < 0 || b < 0) return;
  const [m] = pins.splice(a, 1);
  pins.splice(b, 0, m);
  savePins();
  syncCards();
}

function syncCards() {
  const grid = $('#grid');
  const keep = new Map(cards.map(c => [c.m.key, c]));
  cards.length = 0;
  for (const p of pins) cards.push(keep.get(p.key) || new Card(p));
  grid.replaceChildren(...cards.map(c => c.el));
  renderBoardChrome();
  renderPager();
}

// Mode compact : une carte à la fois (flèches ← → ou boutons de l'en-tête)
function renderPager() {
  const n = cards.length;
  const i = n ? ((settings.compactIdx % n) + n) % n : 0;
  cards.forEach((c, k) => c.el.classList.toggle('cshow', k === i));
  $('#pager').hidden = !settings.compact || n < 2;
  $('#pNum').textContent = `${i + 1}/${n}`;
}
function pageCard(d) {
  settings.compactIdx = (settings.compactIdx || 0) + d;
  saveSettings();
  renderPager();
}

function renderAll() {
  cards.forEach(c => c.render());
  if (preview) preview.render();
  renderTraffic();
}

// ---- Bandeau « clé PRIM » et tableau vide ---------------------------------------------
const SUGGEST = [['Châtelet', 'stop'], ['Gare de Lyon', 'stop'], ['La Défense', 'stop'], ['Grandes gares de Paris', 'paris']];

function renderBoardChrome() {
  const banner = $('#keyBanner');
  if (!settings.apiKey) {
    const inp = el('input', { class: 'inp', type: 'password', placeholder: 'Collez votre clé PRIM', autocomplete: 'off', spellcheck: 'false' });
    const save = () => { const k = inp.value.trim(); if (k) { setKey(k); } };
    inp.addEventListener('keydown', e => { if (e.key === 'Enter') save(); });
    banner.replaceChildren(el('div', { class: 'bi' }, icon('i-key')),
      el('div', { class: 'bt' }, el('b', {}, 'Ajoutez votre clé PRIM gratuite'),
        el('p', {}, 'Elle donne les passages en temps réel du métro, du RER, du tram et des bus, et l’état du trafic. ',
          el('a', { href: '#', onclick: e => { e.preventDefault(); openLink('https://prim.iledefrance-mobilites.fr'); } }, 'Obtenir une clé'),
          ' · les trains SNCF fonctionnent sans clé.')),
      el('div', { class: 'field' }, inp, el('button', { class: 'btn pri', onclick: save }, 'Enregistrer')));
    banner.hidden = false;
  } else banner.hidden = true;

  const empty = $('#empty');
  if (!pins.length) {
    empty.replaceChildren(
      el('div', { class: 'big' }, 'Votre ', el('em', {}, 'tableau'), ' est vide'),
      el('p', {}, 'Cherchez une station, une gare ou un arrêt de bus en haut de la fenêtre, puis épinglez-le : ses prochains passages s’affichent ici, côte à côte.'),
      el('div', { class: 'chips' }, SUGGEST.map(([name, type]) => {
        const model = type === 'paris' ? parisModel() : type === 'gare' ? gareModel(SNCF_STATIONS.find(s => s[0] === name)) : stationModel(findStation(name));
        return model ? el('button', { class: 'sug', onclick: () => openPreview(model) },
          type !== 'stop' ? el('span', { class: 'chip' }, 'Train') : (model.lines || []).slice(0, 3).map(id => badge(id, 'sm')), name) : null;
      })));
    empty.hidden = false;
  } else empty.hidden = true;
}

function setKey(k) {
  settings.apiKey = k;
  saveSettings();
  renderBoardChrome();
  renderAll();
  cards.forEach(c => { c.fetchedAt = 0; });
  traffic.fetchedAt = 0;
  loadTraffic(true);
  toast('Clé PRIM enregistrée');
}

// =============================================================================
//  Recherche : stations (embarquées), gares SNCF, arrêts de bus (open data)
// =============================================================================
const STATIONS = (() => {
  const map = new Map();
  for (const l of NETWORK) {
    if (!LINES[l.id]) continue;   // les stations sans zone d'arrêt (pas encore ouvertes) sont écartées ci-dessous
    for (const [zda, name, , year] of l.stops) {
      if (!zda || year) continue;
      const k = norm(name);
      const s = map.get(k) || map.set(k, { name, key: k, lines: [], refs: [] }).get(k);
      if (!s.lines.includes(l.id)) s.lines.push(l.id);
      if (!s.refs.includes(zda)) s.refs.push(zda);
    }
  }
  for (const s of map.values()) s.lines.sort((a, b) => lineRank(a) - lineRank(b));
  return [...map.values()];
})();
const GARES = SNCF_STATIONS.map(([name, uics]) => ({ name, uics, key: norm(name) }));
const findStation = name => STATIONS.find(s => s.key === norm(name));

function stationModel(s) {
  if (!s) return null;
  const modes = [...new Set(s.lines.map(id => MODE_LABEL[LINES[id].m]))].join(' · ');
  return { type: 'stop', key: 'stop:' + [...s.refs].sort().join(','), name: s.name, sub: modes, refs: s.refs.slice(0, 4), lines: s.lines };
}
function gareModel(g) {
  if (!g) return null;
  const [name, uics] = Array.isArray(g) ? g : [g.name, g.uics];
  return { type: 'gare', key: 'gare:' + uics, name, uics, arrivals: false };
}

function score(key, q) {
  if (key === q) return 0;
  if (key.startsWith(q)) return 1;
  if ((' ' + key).includes(' ' + q)) return 2;
  if (q.split(' ').every(w => (' ' + key).includes(' ' + w))) return 3;
  return 9;
}
function localSearch(list, q, n) {
  return list.map(s => [score(s.key, q), s]).filter(x => x[0] < 9)
    .sort((a, b) => a[0] - b[0] || a[1].name.length - b[1].name.length).slice(0, n).map(x => x[1]);
}

const searchUi = { items: [], sel: -1, seq: 0, online: [], onlineGares: [], q: '' };

function renderResults() {
  const box = $('#results');
  const q = searchUi.q;
  if (!q) { box.hidden = true; $('.search').setAttribute('aria-expanded', 'false'); return; }
  const st = localSearch(STATIONS, q, 7);
  const ga = localSearch(GARES, q, 4);
  const known = new Set(ga.map(g => g.uics));
  const gares = ga.concat(searchUi.onlineGares.filter(g => !known.has(g.uics))).slice(0, 6);
  const parisHit = ['grandes gares de paris', 'gares de paris', 'paris', 'sncf', 'trains'].some(k => score(k, q) < 9);
  const stNames = new Set(st.map(s => s.key));
  const bus = searchUi.online.filter(o => !(o.type !== 'bus' && stNames.has(norm(o.name)))).slice(0, 8);
  const items = [];
  const groups = [];
  const add = (label, list, make) => {
    if (!list.length) return;
    groups.push(el('div', { class: 'rgroup' }, label));
    for (const x of list) {
      const { model, node } = make(x);
      const i = items.length;
      items.push(model);
      groups.push(el('div', { class: 'ritem', role: 'option', 'aria-selected': String(i === searchUi.sel),
        onmousedown: e => { e.preventDefault(); choose(i); } }, node));
    }
  };
  add('Métro · RER · Tram', st, s => ({ model: stationModel(s), node: [
    el('div', { class: 'rname' }, s.name), el('div', { class: 'rlines' }, s.lines.slice(0, 8).map(id => badge(id, 'sm')))] }));
  if (parisHit) gares.unshift({ paris: true });
  add('Gares SNCF', gares, g => g.paris ? ({ model: parisModel(), node: [
    el('span', { class: 'chip' }, 'Train'), el('div', { class: 'rname' }, 'Grandes gares de Paris'),
    el('span', { class: 'rsub' }, PARIS_GARES.map(x => x.short).join(' · '))] }) : ({ model: gareModel(g), node: [
    el('span', { class: 'chip' }, 'Train'), el('div', { class: 'rname' }, g.name), el('span', { class: 'rsub' }, 'Départs · arrivées')] }));
  add('Arrêts de bus et autres', bus, o => ({ model: { type: 'stop', key: 'stop:' + o.zda, name: o.name, sub: o.label + (o.town ? ' · ' + o.town : ''), refs: [o.zda], lines: [], mode: o.type },
    node: [icon(MODE_ICON[o.type] || 'm-bus'), el('div', { class: 'rname' }, o.name), el('span', { class: 'rsub' }, [o.label, o.town].filter(Boolean).join(' · '))] }));
  searchUi.items = items;
  if (!items.length) groups.push(el('div', { class: 'rnote' }, searchUi.pending ? 'Recherche…' : 'Aucun résultat pour « ' + $('#q').value.trim() + ' ».'));
  box.replaceChildren(...groups);
  box.hidden = false;
  $('.search').setAttribute('aria-expanded', 'true');
}

const ZDA_TYPES = { metroStation: ['metro', 'Station de métro'], railStation: ['rer', 'Gare'], onstreetTram: ['tram', 'Arrêt de tram'],
  onstreetBus: ['bus', 'Arrêt de bus'] };
const onlineSearch = debounce(async (q, seq) => {
  const enc = encodeURIComponent(`"${q.replace(/"/g, ' ')}"`);
  const stops = fetch(ODS_IDFM + 'zones-d-arrets/records?limit=30&where=' + enc).then(r => r.json()).then(j =>
    (j.results || []).map(r => {
      const t = ZDA_TYPES[r.zdatype] || ['bus', 'Arrêt'];
      return { zda: String(r.zdaid), name: r.zdaname, town: r.zdatown || '', type: t[0], label: t[1] };
    }).filter(o => o.zda && o.zda !== '0')).catch(() => []);
  const gares = fetch('https://ressources.data.sncf.com/api/explore/v2.1/catalog/datasets/gares-de-voyageurs/records?limit=8&select=nom,codes_uic&where=' + enc)
    .then(r => r.json()).then(j => (j.results || []).map(r => ({ name: r.nom, uics: String(r.codes_uic || '').replace(/[^\d,;]/g, '').replace(/;/g, ',') }))
      .filter(g => g.uics)).catch(() => []);
  const [s, g] = await Promise.all([stops, gares]);
  if (seq !== searchUi.seq) return;
  const rank = { metro: 0, rer: 1, tram: 2, bus: 3 };
  searchUi.online = s.sort((a, b) => rank[a.type] - rank[b.type]);
  searchUi.onlineGares = g;
  searchUi.pending = false;
  renderResults();
}, 350);

function onSearchInput() {
  const raw = $('#q').value;
  searchUi.q = norm(raw);
  searchUi.sel = -1;
  searchUi.online = [];
  searchUi.onlineGares = [];
  const seq = ++searchUi.seq;
  searchUi.pending = searchUi.q.length >= 3;
  if (searchUi.pending) onlineSearch(searchUi.q, seq);
  renderResults();
}

function choose(i) {
  const model = searchUi.items[i];
  if (!model) return;
  $('#q').value = '';
  searchUi.q = '';
  renderResults();
  $('#q').blur();
  openPreview(model);
}

// =============================================================================
//  Fenêtres : aperçu d'un arrêt, détail du trafic, réglages
// =============================================================================
const dlg = $('#dlg');
dlg.addEventListener('click', e => { if (e.target === dlg) dlg.close(); });
let onDialogClose = null;   // réglages fermés sans enregistrer : thème d'origine
dlg.addEventListener('close', () => {
  preview = null;
  if (onDialogClose) { const f = onDialogClose; onDialogClose = null; f(); }
});

function openDialog(title, lead, body, foot, wide) {
  dlg.style.width = wide ? 'min(1000px, calc(100vw - 40px))' : '';
  dlg.replaceChildren(...[
    el('div', { class: 'dlg-head' }, lead, el('h3', {}, title),
      el('button', { class: 'ibtn', title: 'Fermer', onclick: () => dlg.close() }, icon('i-close'))),
    el('div', { class: 'dlg-body' }, body),
    foot ? el('div', { class: 'dlg-foot' }, foot) : null].filter(Boolean));
  if (!dlg.open) dlg.showModal();
}

function openPreview(model) {
  preview = new Card(JSON.parse(JSON.stringify(model)), { preview: true });
  const pinned = isPinned(model.key);
  openDialog(model.type === 'paris' ? 'Grandes gares de Paris' : model.type === 'gare' ? 'Gare' : 'Prochains passages', null, preview.el,
    [el('button', { class: 'btn', onclick: () => dlg.close() }, 'Fermer'),
      el('button', { class: 'btn pri', disabled: pinned, onclick: () => { pin(preview.m); dlg.close(); } },
        pinned ? 'Déjà sur le tableau' : 'Épingler au tableau')], isTrainCard(model));
  preview.refresh();
}

function openLine(id) {
  const li = lineInfo(id) || { n: '?', m: 'bus' };
  const year = futureYear(id);
  if (year) {   // ligne en construction : pas encore de trafic
    const stops = (NETWORK.find(x => x.id === id) || { stops: [] }).stops;
    return openDialog(`${MODE_LABEL[li.m] || ''} ${li.n}`, badge(id, 'lg'), el('div', { class: 'msg l1' },
      el('h4', {}, 'Ligne en construction'),
      el('div', { class: 'when' }, `Ouverture prévue en ${year} · Grand Paris Express`),
      el('p', {}, 'Les prochains passages et l’info trafic apparaîtront ici dès sa mise en service.'),
      el('div', { class: 'stops-list' }, stops.map(st => el('span', {}, st[1])))));
  }
  const items = traffic.items.filter(it => it.line === id);
  const body = items.length ? items.map(it => el('div', { class: 'msg l' + it.level + (it.works ? ' w' : '') },
    el('h4', {}, it.title || it.short),
    el('div', { class: 'when' }, (it.active ? 'En cours · ' : 'À venir · ') + whenText(it.begin, it.end) +
      (it.works ? ' · Travaux' : it.level === 3 ? ' · Trafic interrompu' : it.level === 2 ? ' · Trafic perturbé' : ' · Information')),
    el('p', {}, it.msg || it.short)))
    : el('div', { class: 'state' }, icon('i-check'), el('b', {}, 'Trafic normal'), 'Aucune perturbation annoncée sur cette ligne.');
  const followed = () => (settings.followed || []).includes(id);
  const follow = el('button', { class: 'btn', title: 'Alertes d’incident pour cette ligne (réglage « Mes lignes »)', onclick: () => {
    settings.followed = followed() ? settings.followed.filter(x => x !== id) : [...(settings.followed || []), id];
    saveSettings();
    resetAlerts();
    follow.replaceChildren(icon('i-bell'), followed() ? 'Ligne suivie ✓' : 'Suivre cette ligne');
  } }, icon('i-bell'), followed() ? 'Ligne suivie ✓' : 'Suivre cette ligne');
  openDialog(`${MODE_LABEL[li.m] || ''} ${li.n}`, badge(id, 'lg'), body,
    [follow, el('button', { class: 'btn pri', onclick: () => dlg.close() }, 'Fermer')]);
}

// =============================================================================
//  Alertes : une fenêtre pop-up dès qu'un incident touche une ligne
// =============================================================================
const alertsUi = { known: null, cards: new Map(), clickLine: null };
const lineLabel = id => { const li = lineInfo(id); return li ? `${MODE_LABEL[li.m] || 'Ligne'} ${li.n}` : 'Ligne'; };
const incidentTag = g => (g.works ? 'Travaux · ' : 'Incident · ') + (g.level === 3 ? 'trafic interrompu' : 'trafic perturbé');

// Incidents en cours (trafic perturbé ou interrompu) sur les lignes surveillées, par perturbation
function incidents() {
  const map = new Map();
  if (settings.alerts === 'off') return map;
  let mine = null;
  if (settings.alerts === 'mine') {
    mine = new Set(settings.followed || []);
    for (const c of cards) for (const id of c.lineIds()) mine.add(id);
  }
  for (const it of traffic.items) {
    if (!it.active || it.level < 2 || (mine && !mine.has(it.line))) continue;
    const g = map.get(it.id) || map.set(it.id, { id: it.id, lines: [], level: it.level, works: it.works, title: it.title,
      short: it.short, msg: it.msg, begin: it.begin, end: it.end }).get(it.id);
    if (!g.lines.includes(it.line)) g.lines.push(it.line);
    g.level = Math.max(g.level, it.level);
  }
  return map;
}

function resetAlerts() { alertsUi.known = traffic.updated ? incidents() : null; }   // réglage changé : pas de rappel

function checkAlerts() {   // après chaque lecture de l'info trafic
  const cur = incidents();
  if (alertsUi.known === null) {   // au lancement : rien (les incidents déjà en cours sont dans la colonne Trafic)
    alertsUi.known = cur;
    return;
  }
  for (const [id, g] of cur) {
    const old = alertsUi.known.get(id);
    if (!old || g.level > old.level) {
      showIncident(g, old ? 'worse' : 'new');
      notifyOutside(g, old ? 'Aggravation' : '');
    } else if (alertsUi.cards.has(id)) showIncident(g, alertsUi.cards.get(id).kind, true);   // détails mis à jour
  }
  for (const [id, g] of alertsUi.known) if (!cur.has(id)) { showIncident(g, 'end'); notifyOutside(g, 'Trafic rétabli'); }
  alertsUi.known = cur;
}

function popup(key, cls, ttl, build, keep) {
  const old = alertsUi.cards.get(key);
  const box = $('#alerts');
  const node = el('article', { class: 'alert ' + cls, role: 'alert' });
  const card = { el: node, timer: 0, kind: cls.split(' ')[0] };
  const close = () => {
    clearTimeout(card.timer);
    node.classList.add('out');
    setTimeout(() => { node.remove(); if (alertsUi.cards.get(key) === card) alertsUi.cards.delete(key); }, 240);
  };
  node.append(...build(close).filter(Boolean), el('i', { class: 'alert-life', style: `animation-duration:${ttl}ms` }));
  const arm = () => { clearTimeout(card.timer); card.timer = setTimeout(close, ttl); };
  node.addEventListener('mouseenter', () => clearTimeout(card.timer));
  node.addEventListener('mouseleave', arm);
  arm();
  if (old) { clearTimeout(old.timer); if (keep) { node.style.animation = 'none'; old.el.replaceWith(node); } else old.el.remove(); }
  if (!node.isConnected) box.prepend(node);
  alertsUi.cards.set(key, card);
  [...alertsUi.cards].slice(0, -3).forEach(([k, c]) => { clearTimeout(c.timer); c.el.remove(); alertsUi.cards.delete(k); });   // 3 au plus
}

// ---- Son des alertes : court carillon synthétisé (aucun fichier audio) ----------------------
let audioCtx = null;
const audio = () => (audioCtx = audioCtx || new (window.AudioContext || window.webkitAudioContext)());
['pointerdown', 'keydown'].forEach(ev => document.addEventListener(ev, () => { try { audio().resume(); } catch { /* pas de son */ } }, { once: true }));
function chime(kind, force) {
  if (!settings.sound && !force) return;
  try {
    const ctx = audio();
    if (ctx.state === 'suspended') ctx.resume();
    // incident : trois notes descendantes ; aggravation : deux fois deux notes ; rétabli : deux notes montantes, plus douces
    const notes = kind === 'end' ? [[659.3, 0, .16], [880, .17, .16]]
      : kind === 'worse' ? [[880, 0, .26], [659.3, .15, .26], [880, .34, .26], [659.3, .49, .26]]
        : [[987.8, 0, .26], [784, .19, .26], [587.3, .38, .26]];
    const t0 = ctx.currentTime + .03;
    for (const [f, dt, vol] of notes) {
      for (const [mult, v] of [[1, 1], [2.01, .18], [3, .06]]) {   // fondamentale + harmoniques : timbre de cloche
        const o = ctx.createOscillator(), g = ctx.createGain();
        o.type = 'sine';
        o.frequency.value = f * mult;
        g.gain.setValueAtTime(.0001, t0 + dt);
        g.gain.exponentialRampToValueAtTime(vol * v, t0 + dt + .012);
        g.gain.exponentialRampToValueAtTime(.0001, t0 + dt + (kind === 'end' ? .7 : 1.1));
        o.connect(g).connect(ctx.destination);
        o.start(t0 + dt);
        o.stop(t0 + dt + 1.2);
      }
    }
  } catch { /* son indisponible */ }
}

function showIncident(g, kind, keep) {
  const end = kind === 'end';
  if (!keep) chime(kind);
  popup(g.id, `${kind} l${end ? 0 : g.level}`, end ? 20e3 : 75e3, close => [
    el('header', { class: 'alert-head' }, el('div', { class: 'lbs' }, g.lines.slice(0, 6).map(id => badge(id, 'lg'))),
      el('div', { class: 'alert-tag' }, end ? 'Trafic rétabli' : (kind === 'worse' ? 'Aggravation · ' : '') + incidentTag(g)),
      el('button', { class: 'ibtn', title: 'Fermer', onclick: close }, icon('i-close'))),
    el('h4', {}, g.title || g.short || 'Perturbation'),
    el('div', { class: 'when' }, end ? 'Fin de la perturbation · ' + hm(now()) : whenText(g.begin, g.end)),
    end ? null : el('p', { class: 'alert-msg' }, g.msg || g.short),
    end ? null : el('div', { class: 'alert-foot' }, el('button', { class: 'btn', onclick: () => { close(); openLine(g.lines[0]); } }, 'Voir la ligne')),
  ], keep);
}

// KeoLive n'est pas au premier plan : notification de Windows (ou du navigateur)
function notifyOutside(g, prefix) {
  if (!document.hidden && document.hasFocus()) return;
  const title = g.lines.slice(0, 3).map(lineLabel).join(', ') + ' : ' + (prefix || (g.level === 3 ? 'trafic interrompu' : 'trafic perturbé'));
  const text = [g.title, g.short !== g.title ? g.short : ''].filter(Boolean).join(' · ').slice(0, 230);
  alertsUi.clickLine = g.lines[0];
  if (NATIVE) native('notify', title + '\n' + text);
  else if (window.Notification && Notification.permission === 'granted') {
    const n = new Notification(title, { body: text, icon: $('link[rel=icon]').href });
    n.onclick = () => { window.focus(); openLine(g.lines[0]); };
  }
}
window.keoNotifyClick = () => { if (alertsUi.clickLine) openLine(alertsUi.clickLine); };

function openAbout() {
  const keolis = $('#imgKeolis').cloneNode();
  keolis.hidden = false;
  keolis.removeAttribute('id');
  openDialog('À propos', icon('i-info'), [
    el('div', { class: 'about-top' },
      el('div', { class: 'about-name', 'aria-label': APP.name }, icon('k-keolis', 'kmark'), el('span', { class: 'ktext' }, APP.name.slice(1))),
      el('div', { class: 'about-ver' }, 'Version ' + APP.version)),
    el('dl', { class: 'about-rows' },
      el('dt', {}, 'Propriétaire'), el('dd', {}, APP.owner),
      el('dt', {}, 'Mise en place pour'), el('dd', {}, el('span', { class: 'plate', title: APP.company }, keolis))),
    el('div', { class: 'about' }, 'Prochains passages et état du trafic en Île-de-France, départs et arrivées des trains. ',
      'Données : PRIM et open data Île-de-France Mobilités, SNCF (ODbL), Train Nomade. Polices Barlow (licence OFL). ',
      'Affichage : Microsoft Edge WebView2.'),
  ], el('button', { class: 'btn pri', onclick: () => dlg.close() }, 'Fermer'));
}

function openSettings() {
  const s = Object.assign({}, settings);
  const key = el('input', { class: 'inp', type: 'password', autocomplete: 'off', spellcheck: 'false',
    placeholder: s.apiKey ? 'Enregistrée ••••' + s.apiKey.slice(-4) + ' (vide = inchangée)' : 'Collez votre clé PRIM' });
  const seg = el('div', { class: 'seg ctl' }, [30, 60, 120].map(v => el('button', { 'aria-pressed': String(s.refresh === v),
    onclick: e => { s.refresh = v; seg.querySelectorAll('button').forEach(b => b.setAttribute('aria-pressed', String(b === e.currentTarget))); } }, v + ' s')));
  const themeSeg = el('div', { class: 'seg ctl' }, [['auto', 'Automatique'], ['dark', 'Sombre'], ['light', 'Clair']].map(([v, label]) =>
    el('button', { 'aria-pressed': String(s.theme === v), onclick: e => {
      s.theme = v;
      themeSeg.querySelectorAll('button').forEach(b => b.setAttribute('aria-pressed', String(b === e.currentTarget)));
      settings.theme = v;   // aperçu immédiat
      applyTheme();
    } }, label)));
  const alertSeg = el('div', { class: 'seg ctl' }, [['all', 'Toutes'], ['mine', 'Mes lignes'], ['off', 'Aucune']].map(([v, label]) =>
    el('button', { 'aria-pressed': String(s.alerts === v), onclick: e => {
      s.alerts = v;
      alertSeg.querySelectorAll('button').forEach(b => b.setAttribute('aria-pressed', String(b === e.currentTarget)));
      if (v !== 'off' && !NATIVE && window.Notification && Notification.permission === 'default') Notification.requestPermission();
    } }, label)));
  const themeBefore = settings.theme;
  let saved = false;
  const budget = el('input', { class: 'inp ctl', type: 'number', min: '50', max: '1000000', value: String(s.budget), style: 'min-width:0;width:130px' });
  const sw = (prop, onchange) => {
    const b = el('button', { class: 'sw ctl', role: 'switch', 'aria-checked': String(!!s[prop]) });
    b.addEventListener('click', () => { s[prop] = !s[prop]; b.setAttribute('aria-checked', String(s[prop])); if (onchange) onchange(s[prop]); });
    return b;
  };
  const used = quota.used('prim'), pct = Math.min(100, Math.round(used * 100 / Math.max(1, s.budget)));
  const body = [
    el('div', { class: 'set' }, el('b', {}, 'Clé API PRIM'),
      el('span', {}, 'Gratuite : créez un compte sur ', el('a', { href: '#', onclick: e => { e.preventDefault(); openLink('https://prim.iledefrance-mobilites.fr'); } }, 'prim.iledefrance-mobilites.fr'),
        ', abonnez-vous aux API « Prochains passages (requête unitaire) » et « Messages Info Trafic », puis générez un jeton.'),
      el('div', { class: 'ctl' }, key)),
    el('div', { class: 'set' }, el('b', {}, 'Rafraîchissement'),
      el('span', {}, 'Intervalle minimum entre deux mises à jour d’un arrêt. Il s’allonge tout seul si le quota du jour risque de manquer.'), seg),
    el('div', { class: 'set' }, el('b', {}, 'Quota d’appels par jour'),
      el('span', {}, `Passages : ${used} appel${used > 1 ? 's' : ''} aujourd’hui · info trafic : ${quota.used('trafic')}. Partagé avec l’afficheur si la clé est la même.`,
        el('div', { class: 'meter' }, el('i', { style: `width:${pct}%` }))), budget),
    el('div', { class: 'set' }, el('b', {}, 'Alertes d’incident'),
      el('span', {}, 'Une fenêtre s’ouvre dès qu’un incident touche une ligne, avec le détail ; si KeoLive n’est pas au premier plan, ',
        'Windows affiche une notification. « Mes lignes » : celles des arrêts épinglés et celles que vous suivez (bouton cloche de la fiche d’une ligne).'), alertSeg),
    el('div', { class: 'set' }, el('b', {}, 'Son des alertes'),
      el('span', {}, 'Un court carillon à chaque fenêtre d’alerte : trois notes pour un incident, quatre pour une aggravation, deux pour un trafic rétabli. ',
        el('a', { href: '#', onclick: e => { e.preventDefault(); chime('new', true); } }, 'Écouter')), sw('sound')),
    el('div', { class: 'set' }, el('b', {}, 'Thème'),
      el('span', {}, 'Automatique : comme Windows (clair le jour si Windows est en mode clair). Touche L pour basculer.'), themeSeg),
    el('div', { class: 'set' }, el('b', {}, 'Trains : grandes lignes uniquement'),
      el('span', {}, 'Masquer RER, Transilien et navettes dans les tableaux SNCF.'), sw('mainlineOnly')),
    NATIVE ? el('div', { class: 'set' }, el('b', {}, 'Toujours au premier plan'),
      el('span', {}, 'La fenêtre reste visible au-dessus des autres (touche T).'), sw('onTop')) : null,
    el('div', { class: 'about' }, `${APP.name} ${APP.version} · données PRIM et open data Île-de-France Mobilités, SNCF (ODbL), Train Nomade. `,
      'Raccourcis : Ctrl K recherche · F11 plein écran · M mode compact · T premier plan · L thème clair ou sombre · F5 actualiser · Échap fermer.'),
  ];
  openDialog('Réglages', icon('i-gear'), body, [
    el('button', { class: 'btn', onclick: () => dlg.close() }, 'Annuler'),
    el('button', { class: 'btn pri', onclick: () => {
      saved = true;
      const k = key.value.trim();
      const b = Math.max(50, Math.min(1000000, parseInt(budget.value, 10) || DEFAULTS.budget));
      const keyChanged = k && k !== settings.apiKey;
      const trainsChanged = s.mainlineOnly !== settings.mainlineOnly;
      const alertsChanged = s.alerts !== settings.alerts;
      Object.assign(settings, s, { budget: b });
      if (k) settings.apiKey = k;
      saveSettings();
      applyWindow();
      dlg.close();
      if (alertsChanged) resetAlerts();
      if (keyChanged) setKey(k);
      if (trainsChanged) cards.forEach(c => { if (isTrainCard(c.m) && c.raw) { c.data = parseTrainNomade(c.raw, c.m.arrivals); c.render(); } });
      toast('Réglages enregistrés');
    } }, 'Enregistrer')]);
  onDialogClose = () => { if (!saved && settings.theme !== themeBefore) { settings.theme = themeBefore; applyTheme(); } };
}

// =============================================================================
//  Trafic : colonne de droite et bandeau défilant
// =============================================================================
function renderTraffic() {
  const side = $('#traffic');
  const kids = [el('div', { class: 'tr-head' }, el('h2', {}, 'Trafic'),
    el('span', {}, traffic.fetchedAt && !traffic.err ? 'à ' + hm(traffic.updated || traffic.fetchedAt) : ''))];
  const bad = Object.entries(traffic.status).filter(([, s]) => s.level >= 2);
  if (!settings.apiKey) kids.push(el('div', { class: 'tr-sum na' }, icon('i-key'), 'Clé PRIM nécessaire'));
  else if (traffic.err && !traffic.fetchedAt) kids.push(el('div', { class: 'tr-sum na' }, icon('i-warn'), traffic.err));
  else if (!traffic.updated) kids.push(el('div', { class: 'tr-sum na' }, el('div', { class: 'loader' })));
  else if (!bad.length) kids.push(el('div', { class: 'tr-sum ok' }, icon('i-check'), 'Trafic normal sur tout le réseau'));
  else kids.push(el('div', { class: 'tr-sum bad' }, icon('i-warn'), `${bad.length} ligne${bad.length > 1 ? 's' : ''} perturbée${bad.length > 1 ? 's' : ''}`));

  for (const [mode, label] of [['metro', 'Métro'], ['rer', 'RER'], ['train', 'Transilien'], ['tram', 'Tramway']]) {
    const ids = LINE_ORDER.filter(id => LINES[id].m === mode);
    kids.push(el('div', { class: 'tr-group' }, el('div', { class: 'tr-label' }, label),
      el('div', { class: 'tr-badges' }, ids.map(id => {
        const s = traffic.status[id] || { level: 0 };
        const year = futureYear(id);
        return el('button', { class: year ? 'tb s0 future' : `tb s${s.level}${s.works && s.level < 2 ? ' w' : ''}`,
          title: `${label} ${LINES[id].n}` + (year ? ` · ouverture prévue en ${year}` : ''),
          onclick: () => openLine(id) }, badge(id), el('span', { class: 'dot' }));
      }))));
  }
  const active = traffic.items.filter(it => it.active && it.level >= 2);
  const minor = traffic.items.filter(it => it.active && it.level < 2);
  const soon = traffic.items.filter(it => !it.active);
  if (active.length) kids.push(el('div', { class: 'tr-label' }, 'En ce moment'),
    el('div', { class: 'tr-list' }, active.slice(0, 12).map(trafficItem)));
  if (minor.length || soon.length) kids.push(el('div', { class: 'tr-label', style: 'margin-top:14px' }, 'Informations et travaux'),
    el('div', { class: 'tr-list' }, minor.concat(soon).slice(0, 8).map(trafficItem)));
  side.replaceChildren(...kids);
  renderTicker(active);
}

function trafficItem(it) {
  return el('button', { class: `ti l${it.level}${it.works ? ' w' : ''}`, onclick: () => openLine(it.line) }, badge(it.line),
    el('div', {}, el('div', { class: 'tag' }, it.active ? (it.level === 3 ? 'Interrompu' : it.level === 2 ? 'Perturbé' : it.works ? 'Travaux' : 'Info')
      : 'À venir · ' + hm(it.begin)), el('b', {}, it.short || it.title), it.short ? el('span', { class: 'sub' }, it.title) : null));
}

function renderTicker(active) {
  const t = $('#ticker');
  if (!active.length || document.body.classList.contains('compact')) { t.hidden = true; return; }
  const run = $('#tickerRun');
  const key = active.map(a => a.id).join();
  if (run.dataset.key !== key) {
    run.dataset.key = key;
    run.replaceChildren(...active.map(it => el('span', { class: 'tk', onclick: () => openLine(it.line) }, badge(it.line, 'sm'),
      el('b', {}, it.short || 'Perturbation'), it.title)));
    requestAnimationFrame(() => { run.style.animationDuration = Math.max(20, run.scrollWidth / 90) + 's'; });
  }
  t.hidden = false;
}

// =============================================================================
//  Fenêtre : horloge, compact, premier plan, raccourcis
// =============================================================================
function tickClock() {
  const [h, m] = hm(now()).split(':');
  const c = $('#clock');
  if (c.dataset.v !== h + m) {
    c.dataset.v = h + m;
    c.replaceChildren(h, el('span', { class: 'sep' }, ':'), m);
  }
}

function applyWindow() {
  document.body.classList.toggle('compact', !!settings.compact);
  $('#bCompact').setAttribute('aria-pressed', String(!!settings.compact));
  $('#bCompact').replaceChildren(icon(settings.compact ? 'i-board' : 'i-compact'));
  $('#bCompact').title = settings.compact ? 'Tableau complet (M)' : 'Mode compact (M)';
  $('#bTop').setAttribute('aria-pressed', String(!!settings.onTop));
  renderPager();
  native('ontop', !!settings.onTop);
  renderTicker(traffic.items.filter(it => it.active && it.level >= 2));
}

// ---- Thème : automatique (comme Windows), sombre ou clair ---------------------------------
const lightQuery = window.matchMedia('(prefers-color-scheme: light)');
const currentTheme = () => settings.theme === 'light' || (settings.theme === 'auto' && lightQuery.matches) ? 'light' : 'dark';
function applyTheme() {
  const t = currentTheme();
  document.documentElement.dataset.theme = t;
  const b = $('#bTheme');
  b.replaceChildren(icon(t === 'light' ? 'i-moon' : 'i-sun'));
  b.title = t === 'light' ? 'Passer en thème sombre (L)' : 'Passer en thème clair (L)';
  native('theme', t);   // barre de titre de la fenêtre assortie
}
function toggleTheme() {
  settings.theme = currentTheme() === 'light' ? 'dark' : 'light';
  saveSettings();
  applyTheme();
}
lightQuery.addEventListener('change', () => { if (settings.theme === 'auto') applyTheme(); });

// ---- Plein écran : toute la surface de l'écran, par-dessus la barre des tâches ------------------
let fullscreen = false;
async function setFullscreen(on) {
  if (on && settings.compact) toggleCompact();
  if (NATIVE) fullscreen = (await native('fullscreen', on)) === true;
  else {   // dans le navigateur : plein écran de la page
    try {
      if (on && !document.fullscreenElement) await document.documentElement.requestFullscreen();
      if (!on && document.fullscreenElement) await document.exitFullscreen();
    } catch { /* refusé par le navigateur */ }
    fullscreen = !!document.fullscreenElement;
  }
  settings.fullscreen = fullscreen && NATIVE;   // rouvert en plein écran (application seulement)
  saveSettings();
  renderFullscreen();
}
function renderFullscreen() {
  document.body.classList.toggle('fs', fullscreen);
  const b = $('#bFull');
  b.setAttribute('aria-pressed', String(fullscreen));
  b.replaceChildren(icon(fullscreen ? 'i-unexpand' : 'i-expand'));
  b.title = fullscreen ? 'Quitter le plein écran (F11 ou Échap)' : 'Plein écran (F11)';
}
document.addEventListener('fullscreenchange', () => { if (!NATIVE) { fullscreen = !!document.fullscreenElement; renderFullscreen(); } });
let idleTimer;
document.addEventListener('mousemove', () => {
  document.body.classList.remove('idle');
  clearTimeout(idleTimer);
  if (fullscreen) idleTimer = setTimeout(() => document.body.classList.add('idle'), 3000);
});

function toggleCompact() {
  if (!settings.compact && fullscreen) { setFullscreen(false); }
  settings.compact = !settings.compact;
  saveSettings();
  native('compact', settings.compact);
  applyWindow();
  cards.forEach(c => c.render());
}
function toggleTop() {
  settings.onTop = !settings.onTop;
  saveSettings();
  applyWindow();
  toast(settings.onTop ? 'Toujours au premier plan' : 'Premier plan désactivé');
}

function setupUi() {
  const q = $('#q');
  q.addEventListener('input', onSearchInput);
  q.addEventListener('focus', () => { if (q.value) onSearchInput(); });
  q.addEventListener('blur', () => setTimeout(() => { $('#results').hidden = true; }, 120));
  q.addEventListener('keydown', e => {
    const n = searchUi.items.length;
    if (e.key === 'ArrowDown' && n) { e.preventDefault(); searchUi.sel = (searchUi.sel + 1) % n; renderResults(); }
    else if (e.key === 'ArrowUp' && n) { e.preventDefault(); searchUi.sel = (searchUi.sel - 1 + n) % n; renderResults(); }
    else if (e.key === 'Enter' && n) { e.preventDefault(); choose(Math.max(0, searchUi.sel)); }
    else if (e.key === 'Escape') { q.value = ''; searchUi.q = ''; renderResults(); q.blur(); }
  });
  $('#bSettings').addEventListener('click', openSettings);
  $('#bAbout').addEventListener('click', openAbout);
  $('#bBrand').addEventListener('click', openAbout);
  $('#pPrev').addEventListener('click', () => pageCard(-1));
  $('#pNext').addEventListener('click', () => pageCard(1));
  $('#bCompact').addEventListener('click', toggleCompact);
  $('#bTop').addEventListener('click', toggleTop);
  $('#bTheme').addEventListener('click', toggleTheme);
  $('#bFull').addEventListener('click', () => setFullscreen(!fullscreen));
  if (!NATIVE) $('#bTop').hidden = true;
  document.addEventListener('keydown', e => {
    const typing = /^(INPUT|TEXTAREA)$/.test(document.activeElement.tagName);
    if ((e.ctrlKey && e.key.toLowerCase() === 'k') || (!typing && e.key === '/')) { e.preventDefault(); if (settings.compact) toggleCompact(); q.focus(); q.select(); }
    else if (e.key === 'F11') { e.preventDefault(); setFullscreen(!fullscreen); }
    else if (e.key === 'Escape' && fullscreen && !typing && !dlg.open) setFullscreen(false);
    else if (e.key === 'F5') { e.preventDefault(); cards.forEach(c => { c.fetchedAt = 0; }); loadTraffic(true); toast('Actualisation…', 1200); }
    else if (!typing && !dlg.open && !e.ctrlKey && !e.altKey && e.key.toLowerCase() === 'm') toggleCompact();
    else if (!typing && !dlg.open && settings.compact && (e.key === 'ArrowLeft' || e.key === 'ArrowRight')) pageCard(e.key === 'ArrowLeft' ? -1 : 1);
    else if (!typing && !dlg.open && !e.ctrlKey && !e.altKey && e.key.toLowerCase() === 't' && NATIVE) toggleTop();
    else if (!typing && !dlg.open && !e.ctrlKey && !e.altKey && e.key.toLowerCase() === 'l') toggleTheme();
  });
  document.addEventListener('visibilitychange', () => { if (!document.hidden) loadTraffic(); });
  document.addEventListener('contextmenu', e => { if (!/^(INPUT|TEXTAREA)$/.test(e.target.tagName) && !e.target.closest('.msg')) e.preventDefault(); });
}

// ---- Boucle : horloge chaque seconde, mises à jour échelonnées ------------------------------
let busy = false;
async function tick() {
  tickClock();
  if (tick.n++ % 2 === 0) { cards.forEach(c => { if (c.data) c.render(); }); if (preview && preview.data) preview.render(); }
  if (busy) return;
  if (document.hidden) { loadTraffic(); return; }   // fenêtre réduite : seules les alertes continuent
  busy = true;
  try {
    const list = preview ? [preview, ...cards] : cards;
    for (const c of list) {
      if (c.m.type === 'stop' && !settings.apiKey) continue;
      if (c.due()) { await c.refresh(); await sleep(250); }
    }
    loadTraffic();
  } finally { busy = false; }
}
tick.n = 0;

// =============================================================================
//  Démarrage
// =============================================================================
async function start() {
  if (/[?&]demo\b/.test(location.search)) await demoMode();
  if (!store.get('parisAdded', false)) {   // une fois : les grandes gares de Paris sur le tableau
    if (!pins.some(p => p.type === 'paris')) { pins.push(parisModel()); savePins(); }
    store.set('parisAdded', true);
  }
  setupUi();
  applyTheme();
  native('compact', !!settings.compact);   // la fenêtre suit le dernier mode choisi
  if (NATIVE && settings.fullscreen && !settings.compact) setFullscreen(true);
  applyWindow();
  syncCards();
  renderTraffic();
  tickClock();
  if (!settings.apiKey && !pins.length) setTimeout(() => $('#q').focus(), 300);
  loadTraffic(true);
  setInterval(tick, 1000);
  tick();
}

// ---- Démonstration (développement) : réponses réelles enregistrées, remises à l'heure ----------
async function demoMode() {
  const base = '../../test/data/';
  const load = f => fetch(base + f).then(r => r.text());
  const [ld, rerE, info, ref, bas] = await Promise.all(['prim_ladefense_reel.json', 'prim_rerE_haussmann_reel.json',
    'prim_infotrafic_reel.json', 'ods_referentiel_reel.json', 'bastille_m1.json'].map(load));
  const shiftIso = (txt, delta) => txt.replace(/"(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d+)?Z)"/g, (m, s) => JSON.stringify(new Date(Date.parse(s) + delta).toISOString()));
  const firstTime = txt => [...txt.matchAll(/"ExpectedDepartureTime":\s*"([^"]+)"/g)].map(m => Date.parse(m[1])).sort((a, b) => a - b)[2] || 0;
  const prim = txt => shiftIso(txt, Date.now() + 100e3 - firstTime(txt));
  const info2 = (() => {
    const j = JSON.parse(info);
    const delta = Date.now() - Date.parse(j.lastUpdatedDate || '2026-11-08T18:00:00Z');
    const sh = s => { const t = parseCompact(s) + delta; const p = parisParts(t); const z = n => String(n).padStart(2, '0');
      return `${p.y}${z(p.mo)}${z(p.d)}T${z(p.h)}${z(p.m)}${z(p.s)}`; };
    for (const d of j.disruptions) for (const p of d.applicationPeriods || []) { p.begin = sh(p.begin); p.end = sh(p.end); }
    j.lastUpdatedDate = new Date().toISOString();
    return JSON.stringify(j);
  })();
  let infoNow = info2;
  const realFetch = window.fetch.bind(window);
  window.fetch = async (url, opts) => {
    const u = String(url);
    const ok = body => new Response(body, { status: 200, headers: { 'Content-Type': 'application/json' } });
    if (u.includes('/stop-monitoring')) {
      await sleep(300);
      const ref = decodeURIComponent(u);
      return ok(prim(/:(470548|470549|462395):/.test(ref) ? ld : /:58718:/.test(ref) ? rerE : bas));
    }
    if (u.includes('/disruptions_bulk/')) return ok(infoNow);
    if (u.includes('referentiel-des-lignes')) return ok(ref);
    return realFetch(url, opts);
  };
  if (!settings.apiKey) settings.apiKey = 'demo';
  // incident simulé sur la ligne 1 (essais des alertes) : demoIncident(true) puis demoIncident(false)
  window.demoIncident = on => {
    const j = JSON.parse(infoNow);
    j.disruptions = j.disruptions.filter(d => d.id !== 'demo');
    j.lines = j.lines.filter(l => !(l.impactedObjects || []).some(o => (o.disruptionIds || []).includes('demo')));
    if (on) {
      const z = n => String(n).padStart(2, '0');
      const cmp = t => { const p = parisParts(t); return `${p.y}${z(p.mo)}${z(p.d)}T${z(p.h)}${z(p.m)}${z(p.s)}`; };
      j.disruptions.push({ id: 'demo', applicationPeriods: [{ begin: cmp(Date.now() - 120e3), end: cmp(Date.now() + 5400e3) }],
        cause: 'PERTURBATION', severity: on === 2 ? 'BLOQUANTE' : 'PERTURBEE', title: 'Métro 1 : Colis suspect - ' + (on === 2 ? 'Trafic interrompu' : 'Trafic perturbé'),
        shortMessage: on === 2 ? 'Trafic interrompu' : 'Trafic perturbé',
        message: '<p>Le trafic est ' + (on === 2 ? 'interrompu' : 'ralenti') + ' entre Châtelet et Nation en raison d’un colis suspect à Gare de Lyon.</p><p>Reprise estimée à 09h30. Empruntez la ligne 14 ou le RER A.</p>' });
      j.lines.push({ id: 'line:IDFM:C01371', name: '1', shortName: '1', mode: 'Metro', impactedObjects: [{ type: 'line', id: 'line:IDFM:C01371', name: '1', disruptionIds: ['demo'] }] });
    }
    infoNow = JSON.stringify(j);
    return loadTraffic(true);
  };
}

start();
