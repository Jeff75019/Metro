/* KeoLive — plan de ligne en temps réel (Windows et Android)

   Une ligne à la fois, une voie par sens de circulation, les rames placées à quai ou entre
   deux stations. PRIM ne publie pas la position des rames : on interroge chaque station de
   la ligne (API « Prochains passages », requête unitaire) et chaque passage porte
   l'identifiant du train (DatedVehicleJourneyRef). La prochaine station qu'un train
   atteindra et l'heure prévue le situent : à quai, ou sur l'intervalle qui y mène, où il
   avance au fil du décompte jusqu'à la lecture suivante.

   Fichier commun aux deux applications (copié dans l'APK par android/build.sh) ; il
   s'appuie sur les fonctions de leur app.js : el, primGet, settings, quota, now, hm,
   parisParts, norm, val, lineIdOf, lineInfo, futureLine, LINES, NETWORK, LINE_ORDER,
   MODE_LABEL, openPreview, stationModel, findStation, toast. */
'use strict';

// =============================================================================
//  Topologie : stations rangées en colonnes, une rangée par branche
// =============================================================================
// Colonnes : l'ordre de la ligne, de gauche à droite (ou de haut en bas sur téléphone).
// Rangée 0 : l'itinéraire principal ; une rangée par branche, qui part de sa station
// d'embranchement vers la gauche (side -1) ou vers la droite (side +1).
// Boucles à sens unique : une case { w, e } porte deux stations, l'une desservie
// seulement par les rames qui vont vers la gauche (w), l'autre vers la droite (e).
const PLAN_SPECS = {
  // métro 7 : branche de Mairie d'Ivry depuis Maison Blanche
  C01377: { branches: [{ from: 'Maison Blanche', first: 'Porte de Choisy', side: 1 }] },
  // métro 13 : branche de Saint-Denis - Université depuis La Fourche
  C01383: { branches: [{ from: 'La Fourche', first: 'Guy Môquet', side: -1 }] },
  // métro 10 : boucle d'Auteuil (vers Boulogne par Église d'Auteuil, vers Austerlitz par Mirabeau)
  C01380: { main: ['Boulogne - Pont de Saint-Cloud', 'Boulogne - Jean Jaurès',
    { w: 'Porte d’Auteuil', e: 'Michel Ange-Molitor' }, { w: 'Michel Ange-Auteuil', e: 'Chardon Lagache' },
    { w: 'Église d’Auteuil', e: 'Mirabeau' }, 'Javel - André Citroën', 'Charles Michels', 'Avenue Émile Zola',
    'La Motte Picquet-Grenelle', 'Ségur', 'Duroc', 'Vaneau', 'Sèvres-Babylone', 'Mabillon', 'Odéon', 'Cluny-La Sorbonne',
    'Maubert-Mutualité', 'Cardinal Lemoine', 'Jussieu', 'Gare d’Austerlitz'] },
  // métro 7 bis : boucle de Pré-Saint-Gervais (aller par Place des Fêtes, retour par Danube)
  C01387: { main: ['Louis Blanc', 'Jaurès', 'Bolivar', 'Buttes-Chaumont', 'Botzaris', { w: 'Danube', e: 'Place des Fêtes' }, 'Pré Saint-Gervais'] },
  // tram T4 : Bondy – Gargan – Aulnay-sous-Bois, branche de Montfermeil depuis Gargan
  C01843: { main: ['Bondy', 'La Remise à Jorelle', 'Les Coquetiers', 'Allée de la Tour Rendez-Vous', 'Les Pavillons-sous-Bois', 'Gargan',
    'Lycée Henri Sellier', 'L’Abbaye', 'Freinville-Sevran', 'Rougemont-Chanteloup', 'Aulnay-sous-Bois'],
    branches: [{ from: 'Gargan', side: 1, stations: ['République Marx Dormoy', 'Léon Blum', 'Maurice Audin', 'Clichy-sous-Bois Mairie',
      'Romain Rolland', 'Clichy - Montfermeil', 'Notre-Dame-des-Anges', 'Arboretum', 'Hôpital de Montfermeil'] }] },
  // tram T8 : branche de Villetaneuse-Université depuis Delaunay-Belleville
  C01795: { branches: [{ from: 'Delaunay-Belleville', first: 'Jean Vilar', side: -1 }] },
};
const PLAN_MODES = ['metro', 'tram'];
const PLAN_DEMO = /[?&]demo\b/.test(location.search);
const PLAN_DWELL = 20e3;   // arrêt en station supposé quand les données ne donnent pas l’heure de départ
const planTopos = new Map();

function planTopo(lineId) {
  if (!planTopos.has(lineId)) {
    let t = null;
    try { t = buildPlanTopo(lineId); } catch (e) { console.warn('plan', lineId, e); }
    planTopos.set(lineId, t);
  }
  return planTopos.get(lineId);
}
const planAvailable = id => !!planTopo(id);

function buildPlanTopo(lineId) {
  const li = LINES[lineId], net = NETWORK.find(l => l.id === lineId);
  if (!li || !net || !PLAN_MODES.includes(li.m) || futureLine(lineId)) return null;
  const all = net.stops.filter(s => s[0] && !s[3]);   // stations en service, avec leur zone d'arrêt PRIM
  if (all.length < 2) return null;
  const spec = PLAN_SPECS[lineId] || {};
  if (!spec.main && !spec.branches && all.some(s => s[2] & 1)) return null;   // branche sans description : pas de plan
  const byName = new Map(all.map(s => [norm(s[1]), s]));
  const find = name => {
    const s = byName.get(norm(name));
    if (!s) throw new Error('station inconnue : ' + name);
    return s;
  };
  let slots;
  if (spec.main) slots = spec.main.map(x => typeof x === 'string' ? find(x) : { w: find(x.w), e: find(x.e) });
  else {
    const b = all.findIndex(s => s[2] & 1);
    slots = b < 0 ? all : all.slice(0, b);
  }
  const nodes = [];
  const add = (s, row, col, only) => {
    const n = { i: nodes.length, zda: s[0], name: s[1], row, col, only };
    nodes.push(n);
    return n;
  };
  slots.forEach((s, col) => {
    if (Array.isArray(s)) add(s, 0, col, 0);
    else { add(s.w, 0, col, -1); add(s.e, 0, col, 1); }
  });
  const branches = (spec.branches || []).map((b, k) => {
    const from = find(b.from);
    let st;
    if (b.stations) st = b.stations.map(find);
    else {
      const i = all.findIndex(s => norm(s[1]) === norm(b.first));
      if (i < 0) throw new Error('branche inconnue : ' + b.first);
      let j = i + 1;
      while (j < all.length && !(all[j][2] & 1)) j++;
      st = all.slice(i, j);
    }
    const J = nodes.find(n => n.row === 0 && n.zda === from[0]);
    if (!J) throw new Error('embranchement absent : ' + b.from);
    const row = k + 1;
    return { row, side: b.side, junction: J, nodes: st.map((s, m) => add(s, row, J.col + b.side * (m + 1), 0)) };
  });
  // Liaisons dans chaque sens : next[d] (d = +1 vers la droite, -1 vers la gauche)
  const next = { 1: new Map(), '-1': new Map() }, prev = { 1: new Map(), '-1': new Map() };
  const link = (d, a, b) => {
    (next[d].get(a.i) || next[d].set(a.i, []).get(a.i)).push(b.i);
    (prev[d].get(b.i) || prev[d].set(b.i, []).get(b.i)).push(a.i);
  };
  const served = d => nodes.filter(n => n.row === 0 && (n.only === 0 || n.only === d)).sort((a, b) => d * (a.col - b.col));
  for (const d of [1, -1]) {
    const seq = served(d);
    for (let k = 0; k + 1 < seq.length; k++) link(d, seq[k], seq[k + 1]);
  }
  for (const b of branches) {
    const out = [b.junction, ...b.nodes];
    for (let k = 0; k + 1 < out.length; k++) { link(b.side, out[k], out[k + 1]); link(-b.side, out[k + 1], out[k]); }
  }
  // Missions (pour le mode démonstration) : l'itinéraire principal, et chaque branche avec la
  // partie de l'itinéraire principal opposée à elle
  const services = [];
  for (const d of [1, -1]) services.push(served(d));
  for (const b of branches) {
    const trunk = served(b.side).filter(n => b.side * (n.col - b.junction.col) <= 0);
    const out = [...trunk, ...b.nodes];
    services.push(out, out.slice().reverse());
  }
  const byZda = new Map();
  for (const n of nodes) (byZda.get(n.zda) || byZda.set(n.zda, []).get(n.zda)).push(n);
  const cols = nodes.map(n => n.col);
  const ends = nodes.filter(n => !n.only && (!(next[1].get(n.i) || []).length || !(next[-1].get(n.i) || []).length));
  // correspondances : autres lignes (métro, RER, tram) à une station du même nom
  const corr = new Map();
  for (const l of NETWORK) {
    if (l.id === lineId || !LINES[l.id] || futureLine(l.id)) continue;
    for (const s of l.stops) if (s[0] && !s[3]) {
      const k = norm(s[1]);
      if (byName.has(k)) (corr.get(k) || corr.set(k, []).get(k)).push(l.id);
    }
  }
  for (const v of corr.values()) v.sort((a, b) => lineRank(a) - lineRank(b));
  return { id: lineId, li, nodes, branches, next, prev, services, byZda, ends, corr,
    colMin: Math.min(...cols), colMax: Math.max(...cols), zdas: [...byZda.keys()] };
}

// Noms des terminus : « La Défense ⇄ Château de Vincennes », branches comprises
function planTermini(topo) {
  const main = topo.nodes.filter(n => n.row === 0 && !n.only);
  const a = main.reduce((x, y) => y.col < x.col ? y : x), b = main.reduce((x, y) => y.col > x.col ? y : x);
  const left = [a.name], right = [b.name];
  for (const br of topo.branches) (br.side < 0 ? left : right).push(br.nodes[br.nodes.length - 1].name);
  return left.join(' / ') + ' ⇄ ' + right.join(' / ');
}

// =============================================================================
//  Lecture des passages d'une station, pour une ligne
// =============================================================================
function parsePlanVisits(json, lineId) {
  const del = (((json || {}).Siri || {}).ServiceDelivery || {}).StopMonitoringDelivery || [];
  const out = [];
  for (const d of del) for (const v of d.MonitoredStopVisit || []) {
    const j = v.MonitoredVehicleJourney || {}, c = j.MonitoredCall || {};
    if (lineIdOf(val(j.LineRef) || (j.LineRef && j.LineRef.value)) !== lineId) continue;
    const st = String(c.DepartureStatus || c.ArrivalStatus || '').toLowerCase();
    if (st === 'cancelled') continue;
    const tA = Date.parse(c.ExpectedArrivalTime || c.AimedArrivalTime || '');
    const tD = Date.parse(c.ExpectedDepartureTime || c.AimedDepartureTime || '');
    if (isNaN(tA) && isNaN(tD)) continue;
    // Le métro (RATP) ne donne qu'une heure par station, celle de l'arrivée à quai : on compte alors
    // un arrêt de 20 secondes. Une heure de départ seule : départ d'un terminus, en général.
    const arr = isNaN(tA) ? tD - PLAN_DWELL : tA;
    const dep = isNaN(tD) || tD <= tA ? arr + PLAN_DWELL : tD;
    out.push({ jid: (j.FramedVehicleJourneyRef || {}).DatedVehicleJourneyRef || v.ItemIdentifier || '',
      arr, dep, atStop: c.VehicleAtStop === true,
      dest: val(c.DestinationDisplay) || val(j.DestinationName) || '' });
  }
  return out;
}

// =============================================================================
//  Suivi des rames
//
//  PRIM ne donne pas la position des rames, seulement, pour chaque station, les heures des
//  prochains passages. Les numéros de course ne suivent pas toujours la même rame d'une station
//  à l'autre (relevé réel, ligne 1 : un même numéro annoncé à Gare de Lyon puis, trop tôt, à
//  Châtelet), et les terminus annoncent les arrivées sous des numéros factices. D'où une
//  déduction station par station, qui ne se sert des numéros que comme indice :
//    - à quai : la première rame annoncée est arrivée et n'est pas encore repartie ;
//    - entre deux stations : la prochaine rame annoncée à une station arrive plus tôt que ne le
//      pourrait la première rame annoncée à la station précédente (en lui ajoutant le temps de
//      parcours) : elle a donc déjà quitté la précédente. Sa place : 1 − attente / parcours ;
//    - au terminus de départ : la première rame annoncée, dans les 150 s avant son départ.
//  Les temps de parcours entre stations voisines sont appris des numéros qui concordent.
// =============================================================================
class PlanTracker {
  constructor(topo) {
    this.topo = topo;
    this.data = new Map();     // n° de station -> passages annoncés, chacun avec son sens de marche
    this.runs = new Map();     // 'a>b' -> derniers temps de parcours relevés (arrêt non compris)
    this.tok = new Map();      // marque de sens dans les numéros de course (RATP : .A. / .R.) -> sens
    this.mem = new Map();      // numéro de course -> dernière station où la rame a été vue (embranchements)
    this.last = [];            // rames affichées au tour précédent (identifiants d'affichage stables)
    this.dests = new Map();    // destination annoncée -> station de la ligne
    this.reachC = new Map();
    this.defRun = topo.li.m === 'tram' ? 70e3 : 75e3;   // parcours entre deux stations, avant apprentissage
  }

  // Station de destination annoncée, retrouvée sur la ligne (« La Défense (Grande Arche) »,
  // « Asnières-Gennevilliers Les Courtilles »…)
  destNode(dest) {
    if (this.dests.has(dest)) return this.dests.get(dest);
    const d = norm(dest);
    let dn = d ? this.topo.nodes.find(x => norm(x.name) === d) : null;
    if (d && !dn) {
      let best = '';
      for (const x of this.topo.nodes) {
        const nx = norm(x.name);
        if (nx.length > best.length && (d.startsWith(nx + ' ') || d.endsWith(' ' + nx))) { best = nx; dn = x; }
      }
    }
    this.dests.set(dest, dn || null);
    return dn || null;
  }

  // Sens de marche d'un passage annoncé à la station b : vers sa destination dn
  dirOf(b, v, dn) {
    const { prev } = this.topo;
    if (b.only) return b.only;
    const m = /\.([AR])\./.exec(v.jid);
    let dir = 0;
    if (dn && dn !== b) dir = Math.sign(dn.col - b.col);
    else if (dn === b) {   // la rame termine ici : elle arrive du seul côté où la ligne continue
      const r = (prev[1].get(b.i) || []).length, l = (prev[-1].get(b.i) || []).length;
      dir = r && !l ? 1 : l && !r ? -1 : 0;
    }
    if (dir && m) this.tok.set(m[1], dir);
    else if (!dir && m) dir = this.tok.get(m[1]) || 0;
    return dir;
  }

  // Stations que l'on atteint depuis b en allant dans le sens dir
  reach(b, dir) {
    const k = b.i + '|' + dir;
    let s = this.reachC.get(k);
    if (!s) {
      s = new Set();
      const st = [b.i];
      while (st.length) for (const j of this.topo.next[dir].get(st.pop()) || []) if (!s.has(j)) { s.add(j); st.push(j); }
      this.reachC.set(k, s);
    }
    return s;
  }

  ingest(zda, visits, at) {
    const b = (this.topo.byZda.get(zda) || [])[0];
    if (!b) return;
    const vs = [];
    for (const v of visits) {
      const dn = this.destNode(v.dest), dir = this.dirOf(b, v, dn);
      if (dir) vs.push({ ...v, dir, dn });
    }
    this.data.set(b.i, { at, visits: vs });
    this.learn(b);
  }

  // Temps de parcours : une même course annoncée à deux stations voisines
  learn(b) {
    const { prev, next } = this.topo;
    for (const dir of [1, -1]) {
      const pairs = [...(prev[dir].get(b.i) || []).map(a => [a, b.i]), ...(next[dir].get(b.i) || []).map(c => [b.i, c])];
      for (const [a, c] of pairs) {
        const da = this.data.get(a), dc = this.data.get(c);
        if (!da || !dc) continue;
        for (const v of dc.visits) {
          if (v.dir !== dir || !v.jid) continue;
          const u = da.visits.find(w => w.jid === v.jid && w.dir === dir);
          if (!u) continue;
          const r = v.arr - u.dep, key = a + '>' + c, cur = this.runTime({ i: a }, { i: c });
          if (r < Math.max(15e3, cur * 0.45) || r > Math.min(300e3, cur * 2.2)) continue;
          const s = this.runs.get(key) || [];
          s.push(r);
          if (s.length > 15) s.shift();
          this.runs.set(key, s);
        }
      }
    }
  }
  runTime(a, b) {
    const s = this.runs.get(a.i + '>' + b.i);
    if (!s || !s.length) return this.defRun;
    const o = [...s].sort((x, y) => x - y);
    return o[o.length >> 1];
  }

  // Position des rames à l'instant t
  positions(t) {
    const { nodes, prev, next } = this.topo;
    // passages encore à venir (ou rame à quai), par station et par sens, dans l'ordre
    const lists = new Map();
    for (const [i, d] of this.data) for (const v of d.visits) {
      if (v.dep < t - 5e3 || v.arr - t > 30 * 60e3) continue;
      const k = i + '|' + v.dir;
      if (!lists.has(k)) lists.set(k, []);
      lists.get(k).push(v);
    }
    for (const l of lists.values()) l.sort((a, b) => a.arr - b.arr);
    // première rame annoncée à la station a qui continue vers b (après une séparation de branches :
    // d'après sa destination)
    const firstToward = (a, b, dir) => {
      const l = lists.get(a.i + '|' + dir) || [];
      if ((next[dir].get(a.i) || []).length < 2) return l[0] || null;
      const r = this.reach(b, dir);
      return l.find(v => !v.dn || v.dn === b || r.has(v.dn.i)) || null;
    };

    const out = [];
    for (const [k, l] of lists) {
      const [si, sd] = k.split('|'), b = nodes[+si], dir = +sd;
      const preds = (prev[dir].get(b.i) || []).map(j => nodes[j]);
      const q = l[0], base = { dir, node: b };
      if (q.arr <= t || (q.atStop && q.arr - t < 45e3)) {
        out.push({ ...base, jid: q.jid, dest: q.dest, prev: null, f: 1, quai: true, eta: 0 });
      } else if (!preds.length) {   // terminus de départ : la rame attend à quai
        if (q.dep - t < 150e3) out.push({ ...base, jid: q.jid, dest: q.dest, prev: null, f: 1, quai: true, eta: 0, depart: q.dep - t });
      }
      // les rames annoncées ici, dans l'ordre, tant qu'elles ont dépassé la station précédente
      // (par voie d'arrivée : deux à une jonction) ; au plus deux rames entre deux stations
      const open = new Map(preds.map(a => [a.i, 0]));
      for (const x of l) {
        if (!open.size) break;
        if (x.arr <= t) continue;   // à quai, vue plus haut
        // encore avant une station précédente ? C'est alors la première rame qui y est annoncée.
        // D'après les heures ; le numéro de course ne départage que si les heures s'y prêtent
        // (il est parfois faux)
        const same = preds.find(a => {
          const y = firstToward(a, b, dir), run = this.runTime(a, b);
          return y && (x.arr >= Math.max(y.dep, t) + run - Math.max(25e3, 0.35 * run)
            || (y.jid && y.jid === x.jid && x.arr >= y.arr + 0.5 * run));
        });
        if (same) { open.delete(same.i); continue; }
        // plus loin que le parcours : rame retenue à quai à la précédente (régulation), qui n'y
        // est plus annoncée ; au-delà de quelques minutes, données trop incohérentes. Une deuxième
        // rame sur le même intervalle doit déjà rouler.
        const cands = preds.filter(a => open.has(a.i)
          && x.arr - t <= (open.get(a.i) ? 1.5 * this.runTime(a, b) + 10e3 : 3 * this.runTime(a, b) + 30e3));
        if (!cands.length) break;
        // à une jonction : la voie d'où vient la rame, d'après là où elle a été vue
        const m = x.jid && this.mem.get(x.jid);
        const a = (m && cands.find(c => c.i === m.i || (m.i === b.i && c.i === m.from)))
          || cands.find(c => c.row === b.row) || cands[0];
        if (open.get(a.i)) open.delete(a.i); else open.set(a.i, 1);
        const f = Math.max(0, Math.min(1, 1 - (x.arr - t) / this.runTime(a, b)));
        out.push({ ...base, jid: x.jid, dest: x.dest, prev: a, f, quai: false, eta: x.arr - t });
      }
    }
    // embranchements : où chaque course a été vue en dernier, et d'où elle venait
    for (const p of out) if (p.jid) {
      const o = this.mem.get(p.jid);
      this.mem.set(p.jid, { i: p.node.i, from: p.prev ? p.prev.i : (o && o.i !== p.node.i ? o.i : o && o.from), t });
    }
    if (this.mem.size > 500) for (const [j, m] of this.mem) if (t - m.t > 30 * 60e3) this.mem.delete(j);

    // identifiants d'affichage stables : une rame qui passe d'une station à la suivante garde le
    // sien (et donc son glissement à l'écran), même si les données la numérotent autrement
    const used = new Set(), keys = new Set();
    const take = (p, o) => { used.add(o); p.key = o.key; keys.add(o.key); };
    for (const p of out) {
      const o = this.last.find(o => !used.has(o) && o.dir === p.dir && o.node === p.node && o.quai === p.quai && o.prev === p.prev);
      if (o) take(p, o);
    }
    for (const p of out) if (!p.key) {
      const o = this.last.find(o => !used.has(o) && o.dir === p.dir && (o.node === p.node || (p.prev && o.node === p.prev)));
      if (o) take(p, o);
    }
    let n = 0;
    for (const p of out) if (!p.key) {
      let key = p.jid || 'r';
      while (keys.has(key)) key = (p.jid || 'r') + '~' + (++n);
      p.key = key;
      keys.add(key);
    }
    this.last = out;
    return out;
  }
}

// =============================================================================
//  Simulation (mode démonstration) : des rames qui suivent un horaire régulier
// =============================================================================
class PlanSim {
  constructor(topo) {
    this.topo = topo;
    const branched = topo.branches.length > 0;
    this.trips = topo.services.map((path, s) => {
      const head = branched ? 300e3 : 150e3;   // intervalle par mission
      const hops = path.map((n, k) => k ? 75e3 + ((n.i * 37 + s * 11) % 40) * 1e3 : 0);
      return { s, path, head, hops, phase: (s * 97e3) % head, dwell: 20e3 };
    });
  }
  // horaires d'un départ : arrivée et départ à chaque station du parcours
  times(trip, t0) {
    const out = [];
    let t = t0;
    trip.path.forEach((n, k) => {
      if (k) t += trip.hops[k];
      const arr = k ? t : t0 - 60e3, dep = k === trip.path.length - 1 ? t : (k ? t + trip.dwell : t0);
      out.push({ n, arr, dep });
      if (k) t += trip.dwell;
    });
    return out;
  }
  // rames en service à l'instant t : [{ jid, dest, times }]
  active(t) {
    const out = [];
    for (const trip of this.trips) {
      const total = trip.hops.reduce((a, b) => a + b, 0) + trip.dwell * trip.path.length;
      const k0 = Math.floor((t - total - 120e3 - trip.phase) / trip.head), k1 = Math.ceil((t + 30 * 60e3 - trip.phase) / trip.head);
      for (let k = k0; k <= k1; k++) {
        const t0 = trip.phase + k * trip.head;
        out.push({ jid: `SIM:${this.topo.id}:${trip.s}:${t0}`, dest: trip.path[trip.path.length - 1].name, times: this.times(trip, t0) });
      }
    }
    return out;
  }
  // réponse d'une station : les 4 prochaines rames dans chaque sens
  visits(zda, t) {
    const nodes = this.topo.byZda.get(zda) || [];
    const by = new Map();
    for (const tr of this.active(t)) for (const x of tr.times) {
      if (!nodes.includes(x.n) || x.dep < t || x.arr - t > 30 * 60e3) continue;
      const k = tr.times.indexOf(x), nx = tr.times[k + 1], pv = tr.times[k - 1];
      const dir = nx ? Math.sign(nx.n.col - x.n.col) : Math.sign(x.n.col - (pv ? pv.n.col : x.n.col - 1));
      (by.get(dir) || by.set(dir, []).get(dir)).push({ jid: tr.jid, arr: x.arr, dep: x.dep, atStop: x.arr <= t, dest: tr.dest });
    }
    return [...by.values()].flatMap(v => v.sort((a, b) => a.arr - b.arr).slice(0, 4));
  }
  // position réelle des rames (essais) : { jid: { p, n, f } }
  truth(t) {
    const out = {};
    for (const tr of this.active(t)) {
      const ts = tr.times;
      if (t < ts[0].arr || t > ts[ts.length - 1].dep + 5e3) continue;
      for (let k = 0; k < ts.length; k++) {
        if (t >= ts[k].arr && t <= ts[k].dep) { out[tr.jid] = { n: ts[k].n.i, p: null, f: 1, quai: true }; break; }
        if (ts[k + 1] && t > ts[k].dep && t < ts[k + 1].arr) {
          out[tr.jid] = { p: ts[k].n.i, n: ts[k + 1].n.i, f: (t - ts[k].dep) / (ts[k + 1].arr - ts[k].dep), quai: false };
          break;
        }
      }
    }
    return out;
  }
}

// =============================================================================
//  Lecture périodique de toutes les stations de la ligne
// =============================================================================
class PlanFeed {
  constructor(topo, tracker, onChange) {
    this.topo = topo;
    this.tracker = tracker;
    this.onChange = onChange;
    this.sim = PLAN_DEMO ? new PlanSim(topo) : null;
    this.running = false;
    this.cycleAt = 0;
    this.err = null;
    this.progress = 0;
  }
  get cost() { return this.topo.zdas.length; }

  // Intervalle entre deux lectures complètes : le réglage « Rafraîchissement », allongé si le
  // quota du jour risque de manquer d'ici 20 h (même règle que les cartes du tableau)
  interval() {
    if (this.sim) return 20;
    const left = settings.budget - quota.used('prim');
    if (left < this.cost) return 3600;
    const p = parisParts(now()), sec = p.h * 3600 + p.m * 60 + p.s;
    const end = sec < 20 * 3600 ? 20 * 3600 : 24 * 3600;
    return Math.max(settings.refresh, Math.ceil((end - sec) * this.cost / left));
  }

  start() {
    if (this.running) return;
    this.running = true;
    this.loop();
  }
  stop() {
    this.running = false;
    if (this.wake) this.wake();
  }
  wait(ms) { return new Promise(r => { this.wake = r; setTimeout(r, ms); }); }

  async loop() {
    while (this.running) {
      const t0 = now();
      if (!this.sim && !settings.apiKey) { this.err = { kind: 'nokey', message: 'Clé PRIM nécessaire (réglages)' }; this.onChange(); await this.wait(5e3); continue; }
      if (document.hidden) { await this.wait(3e3); continue; }
      let ok = 0, last = null;
      this.progress = 0;
      for (const zda of this.topo.zdas) {
        if (!this.running) return;
        try {
          const visits = this.sim ? this.sim.visits(zda, now())
            : parsePlanVisits(await primGet('/stop-monitoring?MonitoringRef=' + encodeURIComponent(`STIF:StopArea:SP:${zda}:`)), this.topo.id);
          this.tracker.ingest(zda, visits, now());
          ok++;
        } catch (e) {
          last = e;
          if (['quota', 'key', 'nokey'].includes(e.kind)) break;
        }
        this.progress = (ok + (last ? 1 : 0)) / this.cost;
        await sleep(this.sim ? 10 : 300);   // PRIM : 5 appels par seconde au plus
      }
      if (!this.running) return;
      this.cycleAt = now();
      this.err = ok ? (last ? { kind: 'partial', message: `${this.cost - ok} station(s) sans réponse` } : null) : last;
      this.onChange();
      await this.wait(Math.max(2e3, this.interval() * 1e3 - (now() - t0)));
    }
  }
}

// =============================================================================
//  Dessin
// =============================================================================
const PLAN_CSS = `
.plan-ov{position:fixed;inset:0;z-index:35;display:grid;grid-template-columns:minmax(0,1fr);grid-template-rows:auto auto minmax(0,1fr) auto;background:var(--bg);color:var(--txt);animation:plan-in .2s ease}
@keyframes plan-in{from{opacity:0;transform:translateY(8px)}}
.plan-head{display:flex;align-items:center;gap:12px;padding:12px 18px;border-bottom:1px solid var(--edge);background:var(--panel)}
.plan-head .lb{flex:none}
.plan-ttl{flex:1;min-width:0}
.plan-ttl b{display:block;font:700 26px/1.05 var(--cond);letter-spacing:.04em;text-transform:uppercase;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.plan-ttl span{display:block;font:600 13px var(--sans);color:var(--mut);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;margin-top:2px}
.plan-count{text-align:right;font:600 12px var(--sans);color:var(--mut);white-space:nowrap}
.plan-count b{display:block;font:700 26px/1 var(--cond);color:var(--txt);letter-spacing:.03em}
.plan-pick{height:38px;border-radius:10px;border:1px solid var(--edge2);background:var(--panel2);color:var(--txt);font:600 14px var(--sans);padding:0 8px;max-width:150px}
.plan-x{width:40px;height:40px;border-radius:11px;display:grid;place-items:center;color:var(--mut);flex:none}
.plan-x:hover{background:var(--panel2);color:var(--txt)}
@media (max-width:700px){
  .plan-head{gap:8px;padding:8px 6px 8px 12px}
  .plan-head .lb{--s:34px}
  .plan-ttl b{font-size:21px}
  .plan-ttl span{font-size:12px}
  .plan-count b{font-size:21px}
  .plan-pick{max-width:96px;height:36px;font-size:13px;padding:0 4px}
  .plan-foot{padding:6px 12px 8px;gap:4px 12px;font-size:12px}
  .plan-notice{margin:8px 12px 0}
}
.plan-body{position:relative;overflow:auto;min-height:0;overscroll-behavior:contain;display:flex}
.plan-body svg{display:block;flex:none;margin:auto}
.plan-foot{display:flex;align-items:center;gap:14px;flex-wrap:wrap;padding:8px 18px 10px;border-top:1px solid var(--edge);font:500 12.5px var(--sans);color:var(--mut);background:var(--panel)}
.plan-foot .grow{flex:1}
.plan-foot .warn{color:var(--orange)}
.plan-foot i{display:inline-block;vertical-align:-2px;margin-right:6px;width:22px;height:11px;border-radius:6px}
.plan-load{position:absolute;left:0;top:0;height:3px;background:var(--amber);transition:width .3s;pointer-events:none}
.plan-tip{position:absolute;z-index:3;pointer-events:none;padding:8px 12px;border-radius:12px;background:var(--panel2);border:1px solid var(--edge2);
  box-shadow:0 10px 30px var(--shadow-lg);font:600 13.5px var(--sans);color:var(--txt);white-space:nowrap}
.plan-tip b{display:block;font:700 17px var(--cond);letter-spacing:.02em;text-transform:uppercase}
.plan-tip span{color:var(--time)}
.plan-notice:empty{margin:0;padding:0;border:0}
.plan-notice{margin:12px 18px 0;padding:10px 14px;border-radius:12px;background:rgba(255,140,26,.12);border:1px solid rgba(255,140,26,.3);color:var(--orange);font:600 13px/1.4 var(--sans)}
.plan-sec{font:700 11px var(--sans);letter-spacing:.14em;text-transform:uppercase;fill:var(--dim)}
.pl-name{font-family:var(--cond);font-weight:600;fill:var(--txt);letter-spacing:.02em;cursor:pointer}
.pl-name.term{font-weight:700}
.pl-name:hover{fill:var(--amber)}
.pl-st{fill:var(--panel2);stroke:var(--edge2);stroke-width:2}
.pl-st.term{stroke-width:3}
.pl-tr{transition:transform 1s linear;cursor:pointer}
.pl-tr.nt{transition:none}
.pl-tr .halo{display:none}
.pl-tr.quai .halo{display:block}
.pl-tr.quai .body{stroke:#fff;stroke-width:2}
.pl-dir{font:700 12px var(--sans);letter-spacing:.08em;fill:var(--mut)}
`;

function planCss() {
  if (document.getElementById('planCss')) return;
  const st = document.createElement('style');
  st.id = 'planCss';
  st.textContent = PLAN_CSS;
  document.head.append(st);
}

const SVGNS_PLAN = 'http://www.w3.org/2000/svg';
function sv(tag, attrs, ...kids) {
  const e = document.createElementNS(SVGNS_PLAN, tag);
  for (const k in attrs) if (attrs[k] != null) e.setAttribute(k, attrs[k]);
  for (const c of kids.flat()) if (c != null) e.append(c.nodeType ? c : document.createTextNode(String(c)));
  return e;
}
function svBadge(id, x, y, r) {
  const l = lineInfo(id);
  if (!l) return null;
  const g = sv('g', {});
  g.append(l.m === 'metro' ? sv('circle', { cx: x, cy: y, r, fill: l.c })
    : sv('rect', { x: x - r, y: y - r, width: 2 * r, height: 2 * r, rx: l.m === 'tram' ? 2 : r * .45, fill: l.c }));
  g.append(sv('text', { x, y: y + r * .38, 'text-anchor': 'middle', 'font-size': r * (l.n.length > 2 ? .85 : 1.1), 'font-weight': 700,
    fill: l.t, 'font-family': 'var(--cond)' }, l.n));
  return g;
}

// Disposition : coordonnées d'écran de chaque station, et position d'une rame
//   orient « wide » : la ligne de gauche à droite, voie du haut vers la gauche, voie du bas vers la droite ;
//   orient « tall » : la ligne de haut en bas, voie de gauche vers le haut, voie de droite vers le bas ;
//                     chaque branche dessinée à part, sous l'itinéraire principal.
class PlanLayout {
  constructor(topo, orient, width) {
    this.topo = topo;
    this.orient = orient;
    this.T = orient === 'wide' ? 15 : 13;   // demi-écart entre les deux voies
    const span = topo.colMax - topo.colMin;
    if (orient === 'wide') {
      // marge de droite : la place du nom (écrit en biais) des stations les plus à droite
      const right = 40 + 6.8 * Math.max(...topo.nodes.filter(n => n.col >= topo.colMax - 1).map(n => n.name.length));
      this.x0 = 50;
      this.step = Math.min(110, (Math.max(width, 760) - this.x0 - right) / Math.max(1, span));
      this.W = Math.ceil(this.x0 + span * this.step + right);   // ligne courte : plan à sa taille, centré
      this.rowY = [190];
      topo.branches.forEach((b, k) => this.rowY.push(190 + 150 * (k + 1)));
      this.H = this.rowY[this.rowY.length - 1] + this.T + (topo.branches.length ? 150 : 110);
    } else {
      this.W = Math.max(width, 300);
      this.RH = 46;
      this.xUp = 30;
      this.xDown = 56;
      // itinéraire principal, puis chaque branche à part : en tête, l'embranchement (rappel)
      this.secTop = [26];
      // une rangée par station : dans une boucle à sens unique, les deux stations d'une même
      // case prennent chacune la leur (celle du sens descendant d'abord)
      const main = topo.nodes.filter(n => n.row === 0).sort((a, b) => a.col - b.col || b.only - a.only);
      this.idx = new Map(main.map((n, k) => [n.i, k]));
      let y = 26 + (main.length - 1) * this.RH;
      for (const b of topo.branches) {
        y += 84;
        this.secTop.push(y);
        y += b.nodes.length * this.RH;
      }
      this.H = y + 40;
    }
  }
  // point d'une station dans une section donnée (tall : la section de la rangée, ou la tête d'une branche)
  pt(n, sec = n.row) {
    if (this.orient === 'wide') return { x: this.x0 + (n.col - this.topo.colMin) * this.step, y: this.rowY[n.row] };
    if (sec === 0) return { x: 0, y: this.secTop[0] + this.idx.get(n.i) * this.RH };
    const b = this.topo.branches[sec - 1];
    return { x: 0, y: this.secTop[sec] + (n === b.junction ? 0 : b.nodes.indexOf(n) + 1) * this.RH };
  }
  // sens d'affichage d'une rame : +1 voie du bas (wide) ou de droite (tall), -1 l'autre voie
  side(dir, sec) {
    if (this.orient === 'wide' || sec === 0) return dir;
    const b = this.topo.branches[sec - 1];
    return dir === b.side ? 1 : -1;   // branche : en s'éloignant de l'embranchement, la rame descend
  }
  // position d'écran d'une rame
  train(p) {
    const n = p.node, a = p.prev;
    let sec = n.row;
    if (a && a.row !== n.row) sec = Math.max(a.row, n.row);
    const q = this.pt(n, sec), s = this.side(p.dir, sec);
    let x, y;
    if (!a || p.quai) { x = q.x; y = q.y; }
    else {
      const o = this.pt(a, sec);
      x = o.x + (q.x - o.x) * p.f;
      y = o.y + (q.y - o.y) * p.f;
    }
    if (this.orient === 'wide') return { x, y: y + s * this.T, s, horiz: true };
    return { x: s > 0 ? this.xDown : this.xUp, y, s, horiz: false };
  }
}

class PlanView {
  constructor(host, topo, orient) {
    this.host = host;
    this.topo = topo;
    this.orient = orient;
    this.els = new Map();
    this.tip = el('div', { class: 'plan-tip', hidden: true });
    this.build();
  }

  build() {
    const topo = this.topo, C = topo.li.c;
    const L = this.L = new PlanLayout(topo, this.orient, this.host.clientWidth || 1000);
    // taille en style : les deux applications fixent à 20 px les SVG (leurs icônes)
    const svg = this.svg = sv('svg', { width: L.W, height: L.H, viewBox: `0 0 ${L.W} ${L.H}`, style: `width:${L.W}px;height:${L.H}px` });
    const defs = sv('defs', {}, sv('filter', { id: 'plGlow', x: '-60%', y: '-90%', width: '220%', height: '280%' },
      sv('feGaussianBlur', { stdDeviation: 3, result: 'b' }), sv('feMerge', {}, sv('feMergeNode', { in: 'b' }), sv('feMergeNode', { in: 'SourceGraphic' }))));
    svg.append(defs);
    const gTrack = sv('g', {}), gSt = sv('g', {}), gLab = sv('g', {});
    this.gTrains = sv('g', {});
    svg.append(gTrack, gSt, gLab, this.gTrains);
    const track = pts => gTrack.append(sv('polyline', { points: pts.map(p => p.x + ',' + p.y).join(' '), fill: 'none', stroke: C,
      'stroke-opacity': .32, 'stroke-width': 6, 'stroke-linecap': 'round', 'stroke-linejoin': 'round' }));
    const term = n => !n.only && topo.ends.includes(n);
    const goStation = n => () => { const s = findStation(n.name); if (s) openPreview(stationModel(s)); };

    if (this.orient === 'wide') {
      const { T } = L;
      for (const d of [1, -1]) {   // voies de l'itinéraire principal (les cases à sens unique ne gênent pas : même ligne droite)
        const seq = topo.nodes.filter(n => n.row === 0).sort((a, b) => a.col - b.col);
        track([seq[0], seq[seq.length - 1]].map(n => { const q = L.pt(n); return { x: q.x + (n === seq[0] ? -16 : 16), y: q.y + d * T }; }));
      }
      for (const b of topo.branches) for (const d of [1, -1]) {
        const seq = [b.junction, ...b.nodes].map(n => L.pt(n));
        track(seq.map(q => ({ x: q.x, y: q.y + d * T })));
      }
      // sens de circulation, au bout de l'itinéraire principal
      const last = topo.nodes.filter(n => n.row === 0).reduce((a, b) => b.col > a.col ? b : a), lq = L.pt(last);
      const fq = L.pt(topo.nodes.filter(n => n.row === 0).reduce((a, b) => b.col < a.col ? b : a));
      gLab.append(sv('path', { d: `M${fq.x - 30} ${fq.y - T} l8 -5 v10 z`, fill: 'var(--mut)' }));   // voie du haut : vers la gauche
      gLab.append(sv('path', { d: `M${lq.x + 30} ${lq.y + T} l-8 -5 v10 z`, fill: 'var(--mut)' }));  // voie du bas : vers la droite
      for (const n of topo.nodes) {
        const q = L.pt(n), t = term(n);
        const h = n.only ? 22 : 2 * T + 24, y = n.only ? q.y + n.only * T - 11 : q.y - T - 12;
        gSt.append(sv('rect', { class: 'pl-st' + (t ? ' term' : ''), x: q.x - 8, y, width: 16, height: h, rx: 8, style: t ? `stroke:${C}` : null }));
        const below = n.row > 0 || n.only === 1;
        const ly = below ? q.y + T + (n.only ? 14 : 18) : q.y - T - 20;
        const name = sv('text', { class: 'pl-name' + (t ? ' term' : ''), x: q.x + (below ? -2 : 2), y: ly, 'font-size': 15.5,
          transform: `rotate(${below ? 40 : -40} ${q.x} ${ly})` }, n.name.toUpperCase());
        name.addEventListener('click', goStation(n));
        gLab.append(name);
        if (n.row === 0 && !n.only) (topo.corr.get(norm(n.name)) || []).slice(0, 3).forEach((id, k) => gLab.append(svBadge(id, q.x, q.y + T + 28 + k * 20, 8.5)));
      }
    } else {
      const { xUp, xDown } = L;
      const mainSeq = topo.nodes.filter(n => n.row === 0).sort((a, b) => a.col - b.col);
      for (const x of [xUp, xDown]) track([{ x, y: L.pt(mainSeq[0], 0).y - 16 }, { x, y: L.pt(mainSeq[mainSeq.length - 1], 0).y + 16 }]);
      topo.branches.forEach((b, k) => {
        const sec = k + 1, top = L.pt(b.junction, sec), bot = L.pt(b.nodes[b.nodes.length - 1], sec);
        gLab.append(sv('text', { class: 'plan-sec', x: 10, y: L.secTop[sec] - 30 }, `Branche ${b.nodes[b.nodes.length - 1].name}`));
        for (const x of [xUp, xDown]) track([{ x, y: top.y }, { x, y: bot.y + 16 }]);
        gSt.append(sv('rect', { class: 'pl-st', x: xUp - 13, y: top.y - 7, width: xDown - xUp + 26, height: 14, rx: 7, 'stroke-dasharray': '3 3' }));
        const nm = sv('text', { class: 'pl-name', x: xDown + 26, y: top.y + 5, 'font-size': 14, fill: 'var(--mut)' }, `depuis ${b.junction.name}`);
        gLab.append(nm);
      });
      for (const n of topo.nodes) {
        const q = L.pt(n), t = term(n);
        const both = !n.only;
        const sideX = n.only ? (L.side(n.only, n.row) > 0 ? xDown : xUp) : null;
        gSt.append(sv('rect', { class: 'pl-st' + (t ? ' term' : ''), x: both ? xUp - 13 : sideX - 13, y: q.y - 7, width: both ? xDown - xUp + 26 : 26,
          height: 14, rx: 7, style: t ? `stroke:${C}` : null }));
        // nom en entier si la place le permet (entre les voies et les correspondances), sinon abrégé
        const label = n.name.toUpperCase(), corr = (topo.corr.get(norm(n.name)) || []).slice(0, 3);
        const max = Math.max(12, Math.floor((L.W - xDown - 38 - corr.length * 20 - (n.only ? 84 : 0)) / 8.4));
        const short = label.length > max ? label.slice(0, max - 1).replace(/[\s-]+$/, '') + '…' : label;
        const name = sv('text', { class: 'pl-name' + (t ? ' term' : ''), x: xDown + 26, y: q.y + 6, 'font-size': 17.5 },
          short, n.only ? sv('tspan', { 'font-size': 12, fill: 'var(--mut)', dx: 6 }, n.only === L.side(1, n.row) ? '▼ seulement' : '▲ seulement') : null);
        name.addEventListener('click', goStation(n));
        gLab.append(name);
        corr.forEach((id, k) => gLab.append(svBadge(id, L.W - 16 - k * 20, q.y, 8.5)));
      }
    }
    this.host.replaceChildren(svg, this.tip);
    this.els.clear();
  }

  // Rames : un groupe par identifiant d'affichage, déplacé en douceur (transition d'une seconde)
  update(list) {
    const C = this.topo.li.c, seen = new Set();
    for (const p of list) {
      const q = this.L.train(p);
      let e = this.els.get(p.key);
      if (!e) {
        const g = sv('g', { class: 'pl-tr nt' });
        const body = q.horiz ? sv('rect', { class: 'body', x: -14, y: -7.5, width: 28, height: 15, rx: 7.5, fill: C })
          : sv('rect', { class: 'body', x: -7.5, y: -14, width: 15, height: 28, rx: 7.5, fill: C });
        const halo = q.horiz ? sv('rect', { class: 'halo', x: -16, y: -9.5, width: 32, height: 19, rx: 9.5, fill: 'none', stroke: C, 'stroke-width': 3, filter: 'url(#plGlow)', opacity: .9 })
          : sv('rect', { class: 'halo', x: -9.5, y: -16, width: 19, height: 32, rx: 9.5, fill: 'none', stroke: C, 'stroke-width': 3, filter: 'url(#plGlow)', opacity: .9 });
        const arrow = sv('path', { fill: 'none', stroke: this.topo.li.t === '#ffffff' ? '#fff' : '#111', 'stroke-width': 2.4, 'stroke-linecap': 'round', 'stroke-linejoin': 'round' });
        g.append(halo, body, arrow);
        g.addEventListener('mouseenter', () => this.showTip(p.key));
        g.addEventListener('mouseleave', () => this.hideTip());
        g.addEventListener('click', ev => { ev.stopPropagation(); this.showTip(p.key, true); });
        this.gTrains.append(g);
        e = { g, arrow, s: 0 };
        this.els.set(p.key, e);
        requestAnimationFrame(() => requestAnimationFrame(() => g.classList.remove('nt')));
      }
      if (e.s !== q.s) {   // flèche dans le sens de marche
        e.arrow.setAttribute('d', q.horiz ? (q.s > 0 ? 'M-3 -4 L4 0 L-3 4' : 'M3 -4 L-4 0 L3 4') : (q.s > 0 ? 'M-4 -3 L0 4 L4 -3' : 'M-4 3 L0 -4 L4 3'));
        e.s = q.s;
      }
      e.g.setAttribute('transform', `translate(${q.x.toFixed(1)} ${q.y.toFixed(1)})`);
      e.g.classList.toggle('quai', !!p.quai);
      e.p = p;
      seen.add(p.key);
    }
    for (const [k, e] of this.els) if (!seen.has(k)) { e.g.remove(); this.els.delete(k); }
    if (this.tipFor && this.els.has(this.tipFor)) this.showTip(this.tipFor, this.tipSticky);
    else if (this.tipFor) this.hideTip();
  }

  showTip(key, sticky) {
    const e = this.els.get(key);
    if (!e || !e.p) return;
    const p = e.p, min = Math.max(0, Math.round(p.eta / 60e3));
    const where = p.quai ? (p.depart > 0 ? `Départ de ${p.node.name} dans ${Math.max(1, Math.round(p.depart / 60e3))} min` : `À quai à ${p.node.name}`)
      : `${p.node.name} ${min ? 'dans ' + min + ' min' : 'à l’approche'}`;
    this.tip.replaceChildren(el('b', {}, '→ ' + (p.dest || 'Destination inconnue')), el('span', {}, where));
    this.tip.hidden = false;
    const q = this.L.train(p), hostR = this.host.getBoundingClientRect(), svgR = this.svg.getBoundingClientRect();
    const sx = svgR.width / this.L.W, x = (svgR.left - hostR.left) + q.x * sx + this.host.scrollLeft, y = (svgR.top - hostR.top) + q.y * sx + this.host.scrollTop;
    const tw = this.tip.offsetWidth;
    this.tip.style.left = Math.max(8, Math.min(x - tw / 2, this.host.scrollLeft + this.host.clientWidth - tw - 8)) + 'px';
    this.tip.style.top = (y + (q.horiz && q.s < 0 ? -64 : 24)) + 'px';
    this.tipFor = key;
    this.tipSticky = !!sticky;
  }
  hideTip(force) {
    if (this.tipSticky && !force) return;
    this.tip.hidden = true;
    this.tipFor = null;
    this.tipSticky = false;
  }
}

// =============================================================================
//  Fenêtre du plan
// =============================================================================
const plan = { ov: null, id: null, feed: null, tracker: null, view: null, timer: 0 };
const planIsOpen = () => !!plan.ov;

function planLines() {
  return LINE_ORDER.filter(id => planAvailable(id));
}

function openPlan(lineId) {
  planCss();
  if (typeof dlg !== 'undefined' && dlg.open) dlg.close();
  closePlan();
  const topo = planTopo(lineId);
  if (!topo) { toast('Plan indisponible pour cette ligne'); return; }
  const li = topo.li;
  const pick = el('select', { class: 'plan-pick', 'aria-label': 'Ligne' });
  for (const [mode, label] of [['metro', 'Métro'], ['tram', 'Tram']]) {
    const og = el('optgroup', { label });
    for (const id of planLines()) if (LINES[id].m === mode) og.append(el('option', { value: id, selected: id === lineId }, `${label} ${LINES[id].n}`));
    pick.append(og);
  }
  pick.addEventListener('change', () => openPlan(pick.value));
  const count = el('div', { class: 'plan-count' }, el('b', {}, '…'), 'rames');
  const body = el('div', { class: 'plan-body' });
  const load = el('i', { class: 'plan-load', style: 'width:0' });
  const foot = el('footer', { class: 'plan-foot' });
  const notice = el('div', { class: 'plan-notice' });
  const ov = el('section', { class: 'plan-ov', role: 'dialog', 'aria-label': `Plan de la ligne ${li.n}` },
    el('header', { class: 'plan-head' },
      el('span', { class: `lb ${li.m} lg`, style: `background:${li.c};color:${li.t}` }, li.n),
      el('div', { class: 'plan-ttl' }, el('b', {}, `${MODE_LABEL[li.m]} ${li.n}`), el('span', {}, planTermini(topo))),
      count, pick,
      el('button', { class: 'plan-x', title: 'Fermer (Échap)', onclick: () => closePlan() }, icon('i-close'))),
    notice,
    el('div', { style: 'position:relative;min-height:0;display:grid' }, body, load),
    foot);
  document.body.append(ov);
  document.body.classList.add('plan-open');
  // à l'horizontale s'il reste assez de place entre deux stations pour leurs noms, sinon de haut en bas
  const wide = () => body.clientWidth >= 760 && new PlanLayout(topo, 'wide', body.clientWidth).step >= 26;
  const tracker = new PlanTracker(topo);
  const view = new PlanView(body, topo, wide() ? 'wide' : 'tall');
  Object.assign(plan, { ov, id: lineId, topo, tracker, view, count, foot, load, body, notice });
  body.addEventListener('click', () => plan.view && plan.view.hideTip(true));
  const feed = plan.feed = new PlanFeed(topo, tracker, () => { renderPlanFoot(); planTick(); });
  feed.start();
  renderPlanFoot();
  planTick();
  clearInterval(plan.timer);
  plan.timer = setInterval(planTick, 1000);
  plan.onResize = debounce(() => {
    if (!planIsOpen()) return;
    const o = wide() ? 'wide' : 'tall';
    if (o !== plan.view.orient || Math.abs(plan.view.L.W - body.clientWidth) > 40) {
      plan.view = new PlanView(body, topo, o);
      planTick();
    }
  }, 250);
  window.addEventListener('resize', plan.onResize);
  if (PLAN_DEMO) window.__plan = { topo, tracker, feed, sim: feed.sim };
}

function planTick() {
  if (!planIsOpen()) return;
  const t = now();
  const list = plan.tracker.positions(t);
  plan.view.update(list);
  const right = list.filter(p => p.dir > 0).length, left = list.length - right;
  plan.count.replaceChildren(el('b', {}, String(list.length)), list.length > 1 ? 'rames' : 'rame', el('br'), `◀ ${left} · ${right} ▶`);
  const f = plan.feed;
  plan.load.style.width = f.progress > 0 && f.progress < 1 ? (f.progress * 100).toFixed(1) + '%' : '0';
}

function renderPlanFoot() {
  if (!planIsOpen()) return;
  const f = plan.feed, iv = f.interval(), C = plan.topo.li.c;
  const [toA, toB] = planTermini(plan.topo).split(' ⇄ ');
  const wide = plan.view.orient === 'wide';
  const kids = [
    el('span', {}, el('i', { style: `background:${C};outline:2px solid #fff;outline-offset:-1px` }), 'à quai'),
    el('span', {}, el('i', { style: `background:${C}` }), 'entre deux stations'),
    el('span', {}, wide ? `◀ voie du haut : vers ${toA} · voie du bas : vers ${toB} ▶` : `▲ voie de gauche : vers ${toA} · voie de droite : vers ${toB} ▼`),
    el('span', { class: 'grow' }),
  ];
  if (f.err && f.err.kind !== 'partial') kids.push(el('span', { class: 'warn' }, '⚠ ' + f.err.message));
  else if (f.cycleAt) kids.push(el('span', {}, `Lu à ${hm(f.cycleAt)} · ${f.cost} stations par lecture, toutes les ${iv >= 120 ? Math.round(iv / 60) + ' min' : iv + ' s'}`
    + (f.sim ? ' · démonstration (rames simulées)' : '') + (f.err ? ` · ${f.err.message}` : '')));
  else kids.push(el('span', {}, f.sim ? 'Démonstration : rames simulées' : `Lecture des ${f.cost} stations…`));
  plan.foot.replaceChildren(...kids);
  // quota trop juste pour suivre la ligne : on le dit, avec le moyen d'y remédier
  if (!f.sim && iv > Math.max(180, settings.refresh * 2) && settings.apiKey)
    plan.notice.replaceChildren(
      `Avec le quota réglé (${settings.budget.toLocaleString('fr-FR')} appels par jour), cette ligne n'est relue que toutes les ${Math.round(iv / 60)} min : `,
      `chaque lecture interroge ses ${f.cost} stations. Si votre clé PRIM a été créée avant le 13 mars 2024, elle a droit à 1 000 000 d'appels par jour : `,
      'indiquez ce quota dans les réglages. Sinon, PRIM l’augmente sur demande.');
  else plan.notice.replaceChildren();
}

function closePlan() {
  if (!plan.ov) return;
  if (plan.feed) plan.feed.stop();
  clearInterval(plan.timer);
  window.removeEventListener('resize', plan.onResize);
  plan.ov.remove();
  Object.assign(plan, { ov: null, id: null, feed: null, tracker: null, view: null });
  document.body.classList.remove('plan-open');
}

// Échap ferme le plan ; ← → passent à la ligne précédente ou suivante (écran large)
document.addEventListener('keydown', e => {
  if (!planIsOpen() || (typeof dlg !== 'undefined' && dlg.open)) return;
  const typing = /^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName);
  if (e.key === 'Escape') { e.preventDefault(); e.stopImmediatePropagation(); closePlan(); }
  else if (!typing && (e.key === 'ArrowLeft' || e.key === 'ArrowRight')) {
    e.preventDefault();
    e.stopImmediatePropagation();
    const ids = planLines(), i = ids.indexOf(plan.id);
    openPlan(ids[(i + (e.key === 'ArrowLeft' ? -1 : 1) + ids.length) % ids.length]);
  }
}, true);
