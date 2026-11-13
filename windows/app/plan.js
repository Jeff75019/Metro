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
// arrêt en station supposé quand les données ne donnent pas l’heure de départ
const planDwell = lineId => (LINES[lineId] || {}).m === 'tram' ? 20e3 : 25e3;
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
  const out = [], dwell = planDwell(lineId);
  for (const d of del) for (const v of d.MonitoredStopVisit || []) {
    const j = v.MonitoredVehicleJourney || {}, c = j.MonitoredCall || {};
    if (lineIdOf(val(j.LineRef) || (j.LineRef && j.LineRef.value)) !== lineId) continue;
    const st = String(c.DepartureStatus || c.ArrivalStatus || '').toLowerCase();
    if (st === 'cancelled') continue;
    const tA = Date.parse(c.ExpectedArrivalTime || c.AimedArrivalTime || '');
    const tD = Date.parse(c.ExpectedDepartureTime || c.AimedDepartureTime || '');
    if (isNaN(tA) && isNaN(tD)) continue;
    // Le métro (RATP) ne donne qu'une heure par station, celle de l'arrivée à quai : on compte alors
    // un arrêt de 25 secondes (20 en tram). Une heure de départ seule : départ d'un terminus, en général.
    const arr = isNaN(tA) ? tD - dwell : tA;
    const dep = isNaN(tD) || tD <= tA ? arr + dwell : tD;
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
    this.version = (this.version || 0) + 1;
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
        const f = 1 - (x.arr - t) / this.runTime(a, b);
        // pas encore partie (plus loin que le parcours) : elle attend à quai à la station précédente
        if (f <= 0) out.push({ dir, node: a, jid: x.jid, dest: x.dest, prev: null, f: 1, quai: true, eta: x.arr - t, held: b });
        else out.push({ ...base, jid: x.jid, dest: x.dest, prev: a, f: Math.min(1, f), quai: false, eta: x.arr - t });
      }
    }
    // une rame en attente à quai déjà vue à quai par la station elle-même : la même, une seule fois
    for (let k = out.length - 1; k >= 0; k--) {
      const p = out[k];
      if (p.held && out.some(o => o !== p && !o.held && o.quai && o.node === p.node && o.dir === p.dir)) out.splice(k, 1);
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
//  Rames suivies dans la durée
//
//  La déduction station par station (PlanTracker) donne à chaque instant une image de la ligne,
//  mais bruitée : d'une annonce à l'autre, une rame déduite peut sembler sauter plus loin, se
//  dédoubler ou disparaître. PlanFleet en fait des rames qui durent, comme sur une vraie ligne :
//  chacune roule de station en station sans sauter ni reculer, accélère, freine, s'arrête à quai,
//  ne double pas celle de devant et n'entre pas dans une station encore occupée. Les annonces
//  ne font que régler son allure : chaque rame vise l'heure d'arrivée annoncée à sa prochaine
//  station, attribuée dans l'ordre des rames qui s'y rendent (une annonce trop tôt pour une rame
//  appartient à une rame devant elle). Une rame n'apparaît qu'au terminus de départ, ou quand les
//  annonces en montrent une, absente du plan, pendant plusieurs secondes ; elle disparaît au
//  terminus, ou si plus aucune annonce ne la confirme.
// =============================================================================
const FLEET = {
  minRun: 0.6,       // parcours au plus vite : 60 % du temps habituel entre deux stations
  dwellMin: 15e3,    // arrêt minimal à quai
  gap: 0.12,         // écart minimal entre deux rames sur un même intervalle (part de l'intervalle)
  step: 1000,        // réglage des rames : une fois par seconde
  spawnDelay: 8e3,   // une rame vue par les annonces depuis moins de 8 s n'est pas encore créée
  lost: 60e3,        // une rame que rien ne confirme depuis 60 s est retirée
  maxDwell: 60e3,    // arrêt le plus long en station (une rame repart toujours)
  maxDwellOrigin: 180e3,  // attente la plus longue au terminus de départ
  slew: 6e3,         // l'heure d'arrivée visée bouge d'au plus 6 s à chaque réglage
  headway: 45e3,     // entre le départ d'une rame et l'arrivée de la suivante à la même station
  unseen: 40e3,      // une rame que les annonces ne montrent plus depuis 40 s est retirée
};

// Rame repartie sur un nouvel horaire en cours de route : vitesse constante, puis freinage
function planCruise(s) {
  const t = Math.min(1, Math.max(0, s)), a = 0.22, v = 1 / (1 - a / 2);
  return t <= 1 - a ? v * t : 1 - v * (1 - t) ** 2 / (2 * a);
}

class PlanFleet {
  constructor(tracker) {
    this.tr = tracker;
    this.topo = tracker.topo;
    this.trains = [];
    this.seq = 0;
    this.lastStep = 0;
    this.ver = -1;
    this.cands = new Map();
    this.dwell = planDwell(this.topo.id);
    this.started = 0;
  }
  run(a, b) { return this.tr.runTime(a, b); }
  // heure de départ la plus tardive : une rame ne reste jamais à quai indéfiniment
  latest(T) {
    const origin = !(this.topo.prev[T.dir].get(T.at.i) || []).length;
    return T.arrAt + (origin ? FLEET.maxDwellOrigin : FLEET.maxDwell);
  }

  // Part de la distance parcourue entre la station quittée et la suivante
  prog(T, t) {
    if (!T.to || t <= T.dep) return 0;
    if (t >= T.tArr) return 1;
    if (T.g0 === 0 && T.t0 <= T.dep) return planEase((t - T.dep) / (T.tArr - T.dep));
    return T.g0 + (1 - T.g0) * planCruise((t - T.t0) / (T.tArr - T.t0));
  }

  // Station suivante, vers la destination de la rame (embranchements)
  nextOf(T) {
    const s = (this.topo.next[T.dir].get(T.at.i) || []).map(i => this.topo.nodes[i]);
    if (s.length < 2) return s[0] || null;
    if (T.dest) {
      const hit = s.find(n => n === T.dest || this.tr.reach(n, T.dir).has(T.dest.i));
      if (hit) return hit;
    }
    return s.find(n => n.row === T.at.row) || s[0];
  }

  // Destination : d'après l'annonce de la rame à la station où elle est à quai
  destFrom(T) {
    const d = this.tr.data.get(T.at.i);
    const v = d && d.visits.find(v => v.dir === T.dir && v.dn && Math.abs(v.arr - T.arrAt) < 40e3);
    if (v) { T.dest = v.dn; T.destName = v.dest; }
  }

  // Arrivée : arrêt à quai, puis départ vers la station suivante (ou fin au terminus)
  arrive(T, ta) {
    T.at = T.to;
    T.arrAt = ta;
    this.destFrom(T);
    T.to = this.nextOf(T);
    T.dep = ta + this.dwell;
    T.t0 = T.dep;
    T.g0 = 0;
    T.tArr = T.to ? T.dep + this.run(T.at, T.to) : T.dep;
    T.endAt = T.to ? 0 : ta + this.dwell;
  }

  spawn(o, t) {
    const T = { id: ++this.seq, dir: o.dir, dest: this.tr.destNode(o.dest), destName: o.dest || '', conf: t, seen: t, endAt: 0 };
    if (o.quai) {
      T.at = o.node;
      T.arrAt = o.depart ? t : t - 5e3;
      T.to = o.held || this.nextOf(T);
      T.dep = o.depart ? t + o.depart
        : o.held ? t + Math.min(FLEET.maxDwell / 2, Math.max(5e3, o.eta - this.run(o.node, o.held))) : t + Math.max(5e3, this.dwell / 2);
      T.t0 = T.dep;
      T.g0 = 0;
      T.tArr = T.to ? T.dep + this.run(T.at, T.to) : T.dep;
      if (!T.to) T.endAt = t + this.dwell;
    } else {
      T.at = o.prev;
      T.to = o.node;
      T.arrAt = t - 60e3;
      T.dep = t - 1;
      T.t0 = t;
      T.g0 = planEase(o.f);
      T.tArr = t + Math.max(2e3, o.eta);
    }
    this.trains.push(T);
  }

  // Les annonces d'une station, attribuées dans l'ordre aux rames qui s'y rendent : alignement
  // des deux suites (une annonce peut rester sans rame, une rame sans annonce), au moindre écart
  align(trains, preds, t) {
    const n = trains.length, m = preds.length, INF = 1e12, SKT = 60, SKP = 40;
    const cost = (T, v) => {
      const run = this.run(T.at, T.to);
      const early = t < T.dep ? Math.max(t, T.arrAt + FLEET.dwellMin) + FLEET.minRun * run
        : t + (1 - this.prog(T, t)) * FLEET.minRun * run;
      // plus tôt qu'elle ne peut arriver : rame de devant, ou rame du plan en retard qui doit
      // se hâter (forte pénalité) ; en retard sur l'horaire : plus vraisemblable qu'en avance
      // (à quai, une rame peut attendre ; presque arrivée, elle ne s'arrête pas des minutes en route)
      const late = (early - 15e3 - v.arr) / 1e3;
      const natural = t < T.dep ? Math.max(t, T.arrAt + this.dwell) + run : t + (1 - this.prog(T, t)) * run;
      const d = (v.arr - natural) / 1e3;   // écart à l'arrivée normale (sans retard)
      if (d < 0) return (late > 0 ? 3 * late : 0) - d * 1.5;
      if (t < T.dep) return 0.3 * Math.min(d, 60) + 3 * Math.max(0, d - 60);   // à quai : attendre un peu, pas des minutes
      return d * (0.6 + 1.5 * this.prog(T, t));
    };
    const D = Array.from({ length: n + 1 }, () => new Float64Array(m + 1).fill(INF));
    const B = Array.from({ length: n + 1 }, () => new Int8Array(m + 1));
    D[0][0] = 0;
    for (let i = 0; i <= n; i++) for (let j = 0; j <= m; j++) {
      if (i && D[i - 1][j] + SKT < D[i][j]) { D[i][j] = D[i - 1][j] + SKT; B[i][j] = 1; }
      if (j && D[i][j - 1] + SKP < D[i][j]) { D[i][j] = D[i][j - 1] + SKP; B[i][j] = 2; }
      if (i && j) {
        const c = D[i - 1][j - 1] + cost(trains[i - 1], preds[j - 1]);
        if (c < D[i][j]) { D[i][j] = c; B[i][j] = 3; }
      }
    }
    const out = [];
    for (let i = n, j = m; i || j;) {
      const s = B[i][j];
      if (s === 3) { out.push([trains[i - 1], preds[j - 1]]); i--; j--; }
      else if (s === 1) i--;
      else j--;
    }
    return out;
  }

  // Une rame vise l'heure d'arrivée annoncée à sa prochaine station, sans à-coup
  aim(T, v, t) {
    T.conf = t;
    if (v.dn) { T.dest = v.dn; T.destName = v.dest; }
    const run = this.run(T.at, T.to), slew = FLEET.slew;
    if (t < T.dep) {   // à quai : départ réglé sur l'arrivée annoncée à la suivante
      const want = Math.min(this.latest(T), Math.max(T.arrAt + FLEET.dwellMin, v.arr - run, t));
      T.dep = Math.max(t, Math.min(this.latest(T), T.dep + Math.max(-slew, Math.min(slew, want - T.dep))));
      T.t0 = T.dep;
      T.g0 = 0;
      T.tArr = Math.max(v.arr, T.dep + FLEET.minRun * run);
    } else {
      const g = this.prog(T, t), min = t + (1 - g) * FLEET.minRun * run;
      const want = Math.max(v.arr, min);
      if (Math.abs(want - T.tArr) < 2e3) return;
      T.t0 = t;
      T.g0 = g;
      T.tArr = Math.max(min, T.tArr + Math.max(-slew, Math.min(slew, want - T.tArr)));
    }
  }

  groups() {
    const by = new Map();
    for (const T of this.trains) if (T.to) {
      const k = T.to.i + '|' + T.dir;
      if (!by.has(k)) by.set(k, []);
      by.get(k).push(T);
    }
    return by;
  }

  plan(t) {
    for (const [k, heading] of this.groups()) {
      const [bi, d] = k.split('|').map(Number), dd = this.tr.data.get(bi);
      if (!dd) continue;
      const preds = dd.visits.filter(v => v.dir === d && v.arr > t - 2e3).sort((x, y) => x.arr - y.arr);
      heading.sort((x, y) => x.tArr - y.tArr);
      for (const [T, v] of this.align(heading, preds, t)) this.aim(T, v, t);
    }
  }

  // Pas de dépassement : sur un même intervalle, la rame de devant arrive la première ; à une
  // station, une rame n'arrive qu'après le départ de celle qui la précède
  order(t) {
    for (const [k, list] of this.groups()) {
      const [bi, d] = k.split('|').map(Number);
      // rang de chaque rame : celles qui roulent, de la plus avancée à la moins avancée, puis
      // celles encore à quai à la station précédente, par heure de départ
      const key = T => t >= T.dep ? -this.prog(T, t) : 1 + (T.dep - t) / 1e7;
      const bySeg = new Map();
      for (const T of list) {
        const s = T.at.i;
        if (!bySeg.has(s)) bySeg.set(s, []);
        bySeg.get(s).push(T);
      }
      for (const seg of bySeg.values()) {
        seg.sort((a, b) => key(a) - key(b));
        for (let i = 1; i < seg.length; i++) {
          const L = seg[i - 1], T = seg[i], allowed = L.tArr + FLEET.dwellMin + FLEET.headway;
          if (T.tArr < allowed) this.delay(T, allowed, t);
        }
      }
      // à la station : une à la fois, la suivante 25 s après le départ de la précédente
      list.sort((x, y) => x.tArr - y.tArr);
      let free = 0;
      for (const T of this.trains) if (T.at.i === bi && T.dir === d && (T.to ? t < T.dep : true)) free = Math.max(free, T.to ? T.dep : T.endAt);
      for (const T of list) {
        const allowed = free + FLEET.headway;
        if (T.tArr < allowed) this.delay(T, allowed, t);
        free = T.tArr + FLEET.dwellMin;
      }
    }
  }
  // Retarde l'arrivée d'une rame : encore à quai, elle y reste (dans la limite de l'arrêt maximal) ;
  // en route, elle ralentit
  delay(T, allowed, t) {
    const run = this.run(T.at, T.to);
    if (t < T.dep) {
      T.dep = Math.max(T.dep, Math.min(this.latest(T), allowed - run));
      T.t0 = T.dep;
      T.g0 = 0;
    } else { T.g0 = this.prog(T, t); T.t0 = t; }
    T.tArr = Math.max(allowed, T.t0 + (1 - T.g0) * FLEET.minRun * run);
  }

  // Distance le long de la voie (en stations, positive si b est devant a) entre deux positions
  // { at: station, g: part de l'intervalle vers la suivante, to }, sur au plus 4 stations
  along(a, b, dir) {
    const next = this.topo.next[dir];
    const walk = (from, target) => {
      let front = [from], d = 0;
      const seen = new Set([from.i]);
      while (front.length && d <= 4) {
        if (front.includes(target)) return d;
        const nf = [];
        for (const n of front) for (const j of next.get(n.i) || []) if (!seen.has(j)) { seen.add(j); nf.push(this.topo.nodes[j]); }
        front = nf;
        d++;
      }
      return null;
    };
    const f = walk(a.at, b.at);
    if (f !== null) return f + b.g - a.g;
    const r = walk(b.at, a.at);
    return r !== null ? -(r + a.g - b.g) : null;
  }
  posOf(T, t) {
    const running = T.to && t > T.dep;
    return { at: T.at, g: running ? this.prog(T, t) : 0 };
  }
  obsPos(o) { return o.quai ? { at: o.node, g: 0 } : { at: o.prev, g: planEase(o.f) }; }

  // Les rames que montrent les annonces, associées une à une aux rames du plan (la plus proche
  // le long de la voie, à moins de 1,2 station) ; celles qui restent sans rame : créées après
  // quelques secondes ; les rames du plan que rien ne confirme plus : retirées
  reconcile(t) {
    const init = t - this.started < 25e3, seen = new Set();
    const obs = this.tr.positions(t), pairs = [];
    obs.forEach((o, i) => {
      const po = this.obsPos(o);
      for (const T of this.trains) {
        if (T.dir !== o.dir) continue;
        const d = this.along(this.posOf(T, t), po, o.dir);
        if (d !== null && Math.abs(d) <= 1.8) pairs.push({ i, T, d: Math.abs(d), sd: d });
      }
    });
    pairs.sort((a, b) => a.d - b.d);
    const usedO = new Set(), usedT = new Set();
    for (const { i, T, sd } of pairs) {
      if (usedO.has(i) || usedT.has(T)) continue;
      usedO.add(i);
      usedT.add(T);
      T.seen = t;
      if (sd > 0.6) this.hurry(T, t);   // en retard sur ce que montrent les annonces : elle se hâte
      const o = obs[i];
      if (!T.destName && o.dest) { T.destName = o.dest; T.dest = this.tr.destNode(o.dest); }
    }
    obs.forEach((o, i) => {
      if (usedO.has(i)) return;
      // à moins de 0,6 station d'une rame du plan : c'est elle, vue autrement par une annonce
      const po = this.obsPos(o);
      if (this.trains.some(T => { if (T.dir !== o.dir) return false; const d = this.along(this.posOf(T, t), po, o.dir); return d !== null && Math.abs(d) < 0.8; })) return;
      const k = o.dir + '|' + (o.prev ? o.prev.i : '') + '|' + o.node.i + '|' + (o.quai ? 'q' : 'r');
      seen.add(k);
      if (!this.cands.has(k)) this.cands.set(k, t);
      if (init || t - this.cands.get(k) >= FLEET.spawnDelay) { this.spawn(o, t); this.cands.delete(k); }
    });
    for (const k of [...this.cands.keys()]) if (!seen.has(k)) this.cands.delete(k);
    this.trains = this.trains.filter(T => !(T.endAt && t >= T.endAt) && t - Math.max(T.conf, T.seen) < FLEET.lost
      && t - T.seen < FLEET.unseen);
  }
  // Rame en retard sur les annonces : elle repart dès que possible et roule au plus vite
  hurry(T, t) {
    if (!T.to) return;
    const run = this.run(T.at, T.to);
    if (t < T.dep) {
      T.dep = Math.max(t, T.arrAt + FLEET.dwellMin);
      T.t0 = T.dep;
      T.g0 = 0;
      T.tArr = Math.min(T.tArr, T.dep + FLEET.minRun * run);
    } else {
      const g = this.prog(T, t), fast = t + (1 - g) * FLEET.minRun * run;
      if (fast < T.tArr - 2e3) { T.g0 = g; T.t0 = t; T.tArr = fast; }
    }
  }

  step(t) {
    for (const T of this.trains) {
      let n = 0;
      while (T.to && t >= T.tArr && n++ < 50) this.arrive(T, T.tArr);   // arrivées (et départs) échus
      // à quai à un embranchement : la voie suivante, d'après la destination désormais connue
      if (T.to && t < T.dep && (this.topo.next[T.dir].get(T.at.i) || []).length > 1) {
        const nx = this.nextOf(T);
        if (nx !== T.to) { T.to = nx; T.tArr = T.dep + this.run(T.at, T.to); }
      }
    }
    this.plan(t);
    this.reconcile(t);
    this.order(t);
  }

  positions(t) {
    if (!this.started) this.started = t;
    if (t - this.lastStep >= FLEET.step || this.tr.version !== this.ver) {
      this.ver = this.tr.version;
      this.lastStep = t;
      this.step(t);
    }
    const out = [], runners = new Map();
    for (const T of this.trains) {
      const base = { key: 'r' + T.id, jid: '', dir: T.dir, dest: T.destName || '' };
      if (T.to && t > T.dep) {
        const p = { ...base, node: T.to, prev: T.at, f: this.prog(T, t), dist: true, quai: false, eta: T.tArr - t };
        out.push(p);
        const k = T.at.i + '>' + T.to.i + '|' + T.dir;
        if (!runners.has(k)) runners.set(k, []);
        runners.get(k).push(p);
      } else {
        const origin = !(this.topo.prev[T.dir].get(T.at.i) || []).length;
        out.push({ ...base, node: T.at, prev: null, f: 1, quai: true, eta: 0, depart: origin && T.dep > t ? T.dep - t : undefined });
      }
    }
    // jamais deux rames l'une sur l'autre sur un même intervalle, ni d'entrée dans une station occupée
    const busy = new Set(out.filter(p => p.quai).map(p => p.node.i + '|' + p.dir));
    for (const list of runners.values()) {
      list.sort((a, b) => b.f - a.f);
      let lim = busy.has(list[0].node.i + '|' + list[0].dir) ? 0.8 : 1;
      for (const p of list) {
        if (p.f > lim) p.f = Math.max(0, lim);
        lim = p.f - FLEET.gap;
      }
    }
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
// PRIM garde chaque réponse 30 secondes en cache (mesuré : en-tête X-Cache-Status « Hit », même
// ResponseTimestamp) : relire une station plus souvent redonne la même réponse.
const PLAN_PRIM_CACHE = 30e3;
const planHMS = (() => { const f = new Intl.DateTimeFormat('fr-FR', { timeZone: 'Europe/Paris', hour: '2-digit', minute: '2-digit', second: '2-digit' }); return ms => f.format(new Date(ms)); })();

class PlanFeed {
  constructor(topo, tracker, onChange) {
    this.topo = topo;
    this.tracker = tracker;
    this.onChange = onChange;
    this.sim = PLAN_DEMO ? new PlanSim(topo) : null;
    this.running = false;
    this.due = new Map();      // zone d'arrêt -> heure de sa prochaine lecture
    this.rt = new Map();       // zone d'arrêt -> horodatage PRIM de sa dernière réponse
    this.early = new Map();    // zone d'arrêt -> relectures arrivées avant le renouvellement
    this.skews = [];           // âge des réponses à leur arrivée (horloge locale − horloge de PRIM)
    this.got = new Set();      // stations lues au moins une fois
    this.fails = new Set();    // stations dont la dernière lecture a échoué
    this.err = null;
    this.lastAt = 0;
    this.progress = 0;
    this.notified = 0;
  }
  get cost() { return this.topo.zdas.length; }

  // Période de relecture d'une station : en direct, au rythme où PRIM renouvelle ses données
  // (30 s) ; plus lente si le quota du jour risque de manquer d'ici 20 h (même règle que les cartes
  // du tableau)
  period() {
    if (this.sim) return 20;
    const left = settings.budget - quota.used('prim');
    if (left < this.cost) return 3600;
    const p = parisParts(now()), sec = p.h * 3600 + p.m * 60 + p.s;
    const end = sec < 20 * 3600 ? 20 * 3600 : 24 * 3600;
    return Math.max(PLAN_PRIM_CACHE / 1e3, Math.ceil((end - sec) * this.cost / left));
  }
  get live() { return this.period() * 1e3 <= PLAN_PRIM_CACHE; }

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

  // Une station à la fois : celle dont l'heure de relecture est la plus ancienne
  async loop() {
    while (this.running) {
      if (!this.sim && !settings.apiKey) { this.err = { kind: 'nokey', message: 'Clé PRIM nécessaire (réglages)' }; this.changed(true); await this.wait(5e3); continue; }
      if (document.hidden) { await this.wait(3e3); continue; }
      let zda = null, d = Infinity;
      for (const z of this.topo.zdas) { const u = this.due.get(z) || 0; if (u < d) { d = u; zda = z; } }
      const t = now();
      if (d > t) { await this.wait(Math.min(d - t, 5e3)); continue; }
      await this.read(zda);
      if (!this.running) return;
      await sleep(this.sim ? 10 : 250);   // 4 appels par seconde au plus (PRIM en accepte 5 sur une clé ordinaire)
    }
  }

  async read(zda) {
    try {
      let visits, rt = 0;
      if (this.sim) visits = this.sim.visits(zda, now());
      else {
        const json = await primGet('/stop-monitoring?MonitoringRef=' + encodeURIComponent(`STIF:StopArea:SP:${zda}:`));
        visits = parsePlanVisits(json, this.topo.id);
        rt = Date.parse((((json || {}).Siri || {}).ServiceDelivery || {}).ResponseTimestamp || '') || 0;
      }
      const t = now();
      this.tracker.ingest(zda, visits, t);
      this.got.add(zda);
      this.fails.delete(zda);
      this.err = null;
      this.lastAt = t;
      this.due.set(zda, this.nextRead(zda, rt, t));
    } catch (e) {
      this.fails.add(zda);
      this.err = e;
      this.due.set(zda, now() + (['quota', 'key', 'nokey'].includes(e.kind) ? 60e3 : 15e3));
    }
    this.progress = this.got.size / this.cost;
    this.changed();
  }

  // Prochaine lecture d'une station : juste après que PRIM a renouvelé sa réponse (30 s après
  // l'horodatage de la réponse en cache), ou plus tard si le quota l'exige
  nextRead(zda, rt, t) {
    const per = this.period() * 1e3;
    if (!rt || per > PLAN_PRIM_CACHE) return t + per;
    // écart entre les deux horloges : le plus petit âge relevé (une réponse fraîche n'a que
    // le temps du transfert)
    this.skews.push(t - rt);
    if (this.skews.length > 60) this.skews.shift();
    const skew = Math.min(...this.skews);
    // relue trop tôt (même réponse qu'avant) : on réessaie un peu plus tard, de plus en plus
    const same = this.rt.get(zda) === rt, n = same ? (this.early.get(zda) || 0) + 1 : 0;
    this.rt.set(zda, rt);
    this.early.set(zda, n);
    const fresh = rt + skew + PLAN_PRIM_CACHE + 1e3;
    return Math.max(fresh, t + (n ? Math.min(PLAN_PRIM_CACHE, 4e3 * 2 ** (n - 1)) : 5e3));
  }

  // pied de la fenêtre : redessiné au plus une fois par seconde
  changed(force) {
    const t = Date.now();
    if (!force && t - this.notified < 1000) return;
    this.notified = t;
    this.onChange();
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
.pl-tr{cursor:pointer;transition:opacity .4s}
.pl-tr.nt,.pl-tr.gone{opacity:0}
.pl-tr.gone{pointer-events:none}
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

// Mouvement entre deux stations : accélération au départ, vitesse constante, freinage à l'arrivée
// (profil de vitesse en trapèze ; f : part du temps de parcours écoulée -> part de la distance)
function planEase(f) {
  const t = Math.min(1, Math.max(0, f)), a = 0.22, v = 1 / (1 - a);
  return t < a ? v * t * t / (2 * a) : t <= 1 - a ? v * (t - a / 2) : 1 - v * (1 - t) ** 2 / (2 * a);
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
      const o = this.pt(a, sec), g = p.dist ? p.f : planEase(p.f);
      x = o.x + (q.x - o.x) * g;
      y = o.y + (q.y - o.y) * g;
    }
    if (this.orient === 'wide') return { x, y: y + s * this.T, s, sec, horiz: true };
    return { x: s > 0 ? this.xDown : this.xUp, y, s, sec, horiz: false };
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
    this.ghosts = [];
  }

  // Rames : un groupe par identifiant d'affichage, redessiné à chaque image. La rame suit sa
  // position calculée ; quand une nouvelle prévision la déplace, elle ne saute pas : elle glisse
  // vers sa nouvelle place en une à deux secondes, et ne recule pas (elle attend d'être rattrapée).
  update(list, dt = 0) {
    const C = this.topo.li.c, L = this.L;
    const far = 1.5 * (L.orient === 'wide' ? L.step : L.RH), k = dt ? 1 - Math.exp(-dt / 600) : 1;
    const items = list.map(p => ({ p, q: L.train(p) }));
    // Une rame retirée du plan garde son dessin 3 s à sa place ; si une rame apparaît tout près sur la même voie, elle le reprend
    // (il glisse jusqu'à elle). Sinon il s'efface en fondu, et une rame nouvelle apparaît en fondu.
    const keys = new Set(list.map(p => p.key)), tnow = performance.now();
    for (const [key, e] of this.els) if (!keys.has(key)) {
      this.els.delete(key);
      e.lost = tnow;
      this.ghosts.push(e);
      if (this.tipFor === key) this.hideTip(true);
    }
    for (const { p, q } of items) {
      if (this.els.has(p.key) || !this.ghosts.length) continue;
      let best = -1, bd = far;
      this.ghosts.forEach((e, i) => {
        const d = Math.hypot(e.x - q.x, e.y - q.y);
        if (e.s === q.s && e.sec === q.sec && d < bd) { bd = d; best = i; }
      });
      if (best >= 0) this.els.set(p.key, this.ghosts.splice(best, 1)[0]);
    }
    this.ghosts = this.ghosts.filter(e => {
      if (tnow - e.lost < 3e3) return true;
      e.g.classList.add('gone');
      setTimeout(() => e.g.remove(), 450);
      return false;
    });
    for (const { p, q } of items) {
      let e = this.els.get(p.key);
      if (!e) {
        const g = sv('g', { class: 'pl-tr nt' });
        const body = q.horiz ? sv('rect', { class: 'body', x: -14, y: -7.5, width: 28, height: 15, rx: 7.5, fill: C })
          : sv('rect', { class: 'body', x: -7.5, y: -14, width: 15, height: 28, rx: 7.5, fill: C });
        const halo = q.horiz ? sv('rect', { class: 'halo', x: -16, y: -9.5, width: 32, height: 19, rx: 9.5, fill: 'none', stroke: C, 'stroke-width': 3, filter: 'url(#plGlow)', opacity: .9 })
          : sv('rect', { class: 'halo', x: -9.5, y: -16, width: 19, height: 32, rx: 9.5, fill: 'none', stroke: C, 'stroke-width': 3, filter: 'url(#plGlow)', opacity: .9 });
        const arrow = sv('path', { fill: 'none', stroke: this.topo.li.t === '#ffffff' ? '#fff' : '#111', 'stroke-width': 2.4, 'stroke-linecap': 'round', 'stroke-linejoin': 'round' });
        g.append(halo, body, arrow);
        // l'identifiant d'une rame peut changer : on cherche celui de son dessin au moment du geste
        const keyOf = () => { for (const [key, x] of this.els) if (x.g === g) return key; return null; };
        g.addEventListener('mouseenter', () => this.showTip(keyOf()));
        g.addEventListener('mouseleave', () => this.hideTip());
        g.addEventListener('click', ev => { ev.stopPropagation(); this.showTip(keyOf(), true); });
        this.gTrains.append(g);
        e = { g, arrow, s: 0, x: q.x, y: q.y, sec: q.sec };
        this.els.set(p.key, e);
        requestAnimationFrame(() => requestAnimationFrame(() => g.classList.remove('nt')));
      }
      const dx = q.x - e.x, dy = q.y - e.y, d = Math.hypot(dx, dy);
      if (!q.horiz && q.sec !== e.sec) { e.x = q.x; e.y = q.y; }   // vers une autre section (vertical) : sans glisser
      else if (d <= 4) { e.x = q.x; e.y = q.y; }                    // mouvement ordinaire : la rame suit
      else if ((q.horiz ? dx : dy) * q.s < 0 && d < far) { /* en arrière : elle attend */ }
      else { e.x += dx * k; e.y += dy * k; }                        // correction : elle glisse
      e.sec = q.sec;
      if (e.s !== q.s) {   // flèche dans le sens de marche
        e.arrow.setAttribute('d', q.horiz ? (q.s > 0 ? 'M-3 -4 L4 0 L-3 4' : 'M3 -4 L-4 0 L3 4') : (q.s > 0 ? 'M-4 -3 L0 4 L4 -3' : 'M-4 3 L0 -4 L4 3'));
        e.s = q.s;
      }
      e.g.setAttribute('transform', `translate(${e.x.toFixed(1)} ${e.y.toFixed(1)})`);
      if (e.quai !== !!p.quai) { e.quai = !!p.quai; e.g.classList.toggle('quai', e.quai); }
      e.p = p;
    }
    // info-bulle : suit sa rame, quatre fois par seconde
    if (this.tipFor && this.els.has(this.tipFor)) { if (performance.now() - (this.tipAt || 0) > 250) this.showTip(this.tipFor, this.tipSticky); }
    else if (this.tipFor) this.hideTip(true);
  }

  showTip(key, sticky) {
    const e = this.els.get(key);
    if (!e || !e.p) return;
    const p = e.p, min = Math.max(0, Math.round(p.eta / 60e3));
    const where = p.quai ? (p.depart > 0 ? `Départ de ${p.node.name} dans ${Math.max(1, Math.round(p.depart / 60e3))} min` : `À quai à ${p.node.name}`)
      : `${p.node.name} ${min ? 'dans ' + min + ' min' : 'à l’approche'}`;
    this.tip.replaceChildren(el('b', {}, '→ ' + (p.dest || 'Destination inconnue')), el('span', {}, where));
    this.tip.hidden = false;
    const q0 = this.L.train(p), q = { ...q0, x: e.x != null ? e.x : q0.x, y: e.y != null ? e.y : q0.y };
    const hostR = this.host.getBoundingClientRect(), svgR = this.svg.getBoundingClientRect();
    this.tipAt = performance.now();
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
  const tracker = new PlanTracker(topo), fleet = new PlanFleet(tracker);
  const view = new PlanView(body, topo, wide() ? 'wide' : 'tall');
  Object.assign(plan, { ov, id: lineId, topo, tracker, fleet, view, count, foot, load, body, notice });
  body.addEventListener('click', () => plan.view && plan.view.hideTip(true));
  const feed = plan.feed = new PlanFeed(topo, tracker, () => { renderPlanFoot(); planTick(); });
  feed.start();
  renderPlanFoot();
  planTick();
  clearInterval(plan.timer);
  plan.timer = setInterval(planTick, 1000);
  cancelAnimationFrame(plan.raf);
  plan.lastFrame = 0;
  plan.list = null;
  plan.raf = requestAnimationFrame(planFrame);
  plan.onResize = debounce(() => {
    if (!planIsOpen()) return;
    const o = wide() ? 'wide' : 'tall';
    if (o !== plan.view.orient || Math.abs(plan.view.L.W - body.clientWidth) > 40) {
      plan.view = new PlanView(body, topo, o);
      planTick();
    }
  }, 250);
  window.addEventListener('resize', plan.onResize);
  if (PLAN_DEMO) window.__plan = { topo, tracker, fleet, feed, sim: feed.sim };
}

// Une image : la position des rames à cet instant, dessinée (30 images par seconde)
function planFrame(ts) {
  if (!planIsOpen()) return;
  plan.raf = requestAnimationFrame(planFrame);
  if (plan.lastFrame && ts - plan.lastFrame < 30) return;
  const dt = plan.lastFrame ? Math.min(250, ts - plan.lastFrame) : 0;
  plan.lastFrame = ts;
  plan.list = plan.fleet.positions(now());
  plan.view.update(plan.list, dt);
}

// Une fois par seconde : nombre de rames, barre de chargement
function planTick() {
  if (!planIsOpen()) return;
  const list = plan.list || plan.fleet.positions(now());
  const right = list.filter(p => p.dir > 0).length, left = list.length - right;
  plan.count.replaceChildren(el('b', {}, String(list.length)), list.length > 1 ? 'rames' : 'rame', el('br'), `◀ ${left} · ${right} ▶`);
  const f = plan.feed;
  plan.load.style.width = f.progress > 0 && f.progress < 1 ? (f.progress * 100).toFixed(1) + '%' : '0';
}

function renderPlanFoot() {
  if (!planIsOpen()) return;
  const f = plan.feed, iv = f.period(), C = plan.topo.li.c;
  const [toA, toB] = planTermini(plan.topo).split(' ⇄ ');
  const wide = plan.view.orient === 'wide';
  const kids = [
    el('span', {}, el('i', { style: `background:${C};outline:2px solid #fff;outline-offset:-1px` }), 'à quai'),
    el('span', {}, el('i', { style: `background:${C}` }), 'entre deux stations'),
    el('span', {}, wide ? `◀ voie du haut : vers ${toA} · voie du bas : vers ${toB} ▶` : `▲ voie de gauche : vers ${toA} · voie de droite : vers ${toB} ▼`),
    el('span', { class: 'grow' }),
  ];
  const all = f.fails.size >= f.cost || (f.err && ['quota', 'key', 'nokey'].includes(f.err.kind));
  if (f.err && all) kids.push(el('span', { class: 'warn' }, '⚠ ' + f.err.message));
  else if (f.lastAt) kids.push(el('span', {}, (f.live ? (f.sim ? 'Démonstration (rames simulées)' : 'En direct : chaque station relue dès que PRIM renouvelle ses données (toutes les 30 s)')
    : `Chaque station relue toutes les ${iv >= 120 ? Math.round(iv / 60) + ' min' : iv + ' s'} (quota du jour)`)
    + ` · dernière réponse à ${planHMS(f.lastAt)}` + (f.fails.size ? ` · ${f.fails.size} station(s) sans réponse` : '')));
  else kids.push(el('span', {}, f.sim ? 'Démonstration : rames simulées' : `Lecture des ${f.cost} stations…`));
  plan.foot.replaceChildren(...kids);
  // quota trop juste pour suivre la ligne en direct : on le dit, avec le moyen d'y remédier
  if (!f.sim && iv > 120 && settings.apiKey)
    plan.notice.replaceChildren(
      `Avec le quota réglé (${settings.budget.toLocaleString('fr-FR')} appels par jour), chaque station de cette ligne n'est relue que toutes les ${Math.round(iv / 60)} min, au lieu de toutes les 30 s en direct : `,
      `le plan interroge ses ${f.cost} stations. Si votre clé PRIM a droit à plus (jusqu'à 1 000 000 d'appels par jour, sur demande à PRIM ; d'office pour une clé créée avant le 13 mars 2024), `,
      'indiquez ce quota dans les réglages.');
  else plan.notice.replaceChildren();
}

function closePlan() {
  if (!plan.ov) return;
  if (plan.feed) plan.feed.stop();
  clearInterval(plan.timer);
  cancelAnimationFrame(plan.raf);
  window.removeEventListener('resize', plan.onResize);
  plan.ov.remove();
  Object.assign(plan, { ov: null, id: null, feed: null, tracker: null, fleet: null, view: null, list: null });
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
