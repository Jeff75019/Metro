// Page web de configuration, servie par l'afficheur (http://192.168.4.1).
#pragma once

static const char PORTAL_HTML[] = R"HTML(<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Afficheur Métro</title>
<style>
:root{--bg:#06101f;--card:#0e1e38;--line:#1e3357;--txt:#fff;--mut:#8a9bbc;--acc:#ffc629;--ok:#48d597;--err:#ff7a5f}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--txt);font:16px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:640px;margin:0 auto;padding:16px}h1{font-size:22px;margin:4px 0 2px}h2{font-size:17px;margin:0 0 10px;color:var(--acc)}
.sub{color:var(--mut);font-size:14px;margin-bottom:14px}section{background:var(--card);border-radius:14px;padding:16px;margin:14px 0}
label{display:block;font-size:14px;color:var(--mut);margin:10px 0 4px}
input,select{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--line);background:#081429;color:var(--txt);font-size:16px}
input[type=checkbox]{width:auto;margin-right:8px}input[type=range]{padding:0}
button{border:0;border-radius:10px;padding:11px 14px;font-size:15px;font-weight:600;background:var(--line);color:var(--txt);cursor:pointer}
button.pri{background:var(--acc);color:#111}button.sm{padding:6px 10px;font-size:13px}button:disabled{opacity:.5}
.row{display:flex;gap:8px;align-items:center}.row>*{flex:1}.row>button{flex:0 0 auto}
.list>div{display:flex;align-items:center;gap:10px;padding:10px 0;border-bottom:1px solid var(--line)}.list>div:last-child{border:0}
.grow{flex:1;min-width:0}.mut{color:var(--mut);font-size:13px}.ok{color:var(--ok)}.err{color:var(--err)}
.badge{display:inline-flex;align-items:center;justify-content:center;min-width:30px;height:30px;padding:0 6px;border-radius:15px;font-weight:700;font-size:15px}
.badge.sq{border-radius:7px}.badge.rect{border-radius:4px}
.pick{cursor:pointer}.pick:hover{background:#13284a}.chips{display:flex;flex-wrap:wrap;gap:8px;margin-top:8px}
.chip{border:1px solid var(--line);border-radius:20px;padding:6px 12px;cursor:pointer;font-size:14px;background:#081429}.chip.on{border-color:var(--acc);color:var(--acc)}
details summary{cursor:pointer;color:var(--mut);margin-top:12px}.foot{display:flex;gap:10px;position:sticky;bottom:0;padding:12px 0;background:var(--bg)}
.foot button{flex:1}a{color:var(--acc)}#toast{min-height:22px;margin-top:6px}
</style></head><body><main>
<h1>Afficheur Métro</h1><div class="sub" id="st">Chargement…</div>

<section><h2>1. WiFi</h2>
<div class="mut">Le réseau que l'afficheur utilisera pour récupérer les horaires (jusqu'à 3 : maison, bureau, partage de connexion…).</div>
<div class="list" id="wl"></div>
<div class="row" style="margin-top:10px"><button id="scan">Rechercher les réseaux</button></div>
<div class="list" id="scanres"></div>
<label>Nom du réseau (SSID)</label><input id="nssid" autocomplete="off">
<label>Mot de passe</label><input id="npass" type="password" autocomplete="off">
<div class="row" style="margin-top:10px"><button class="pri" id="addw">Ajouter et se connecter</button></div>
<div id="wmsg" class="mut"></div>
</section>

<section><h2>2. Clé API PRIM</h2>
<div class="mut">Gratuite : créez un compte sur <a href="https://prim.iledefrance-mobilites.fr" target="_blank">prim.iledefrance-mobilites.fr</a>, puis « Mes jetons d'authentification » → « Générer un jeton ». Abonnez-vous à l'API « Prochains passages (requête unitaire) ».</div>
<label>Clé (jeton)</label><input id="key" type="password" autocomplete="off">
</section>

<section><h2>3. Mes arrêts</h2>
<div class="mut">Glissez le doigt sur l'écran pour passer d'un arrêt à l'autre (4 maximum).</div>
<div class="list" id="fl"></div>
<label>Ajouter : nom de la station ou de l'arrêt</label>
<div class="row"><input id="q" placeholder="ex. Bastille, Châtelet, Gare de Lyon"><button id="go">Chercher</button></div>
<div id="smsg" class="mut"></div><div class="list" id="sres"></div>
<div id="lsel" style="display:none"><label id="ltitle"></label><div class="chips" id="lchips"></div>
<label>Sens (facultatif)</label><div class="chips" id="dchips"></div>
<label>Nom affiché (facultatif)</label><input id="flabel" placeholder="ex. Maison → Boulot">
<div class="row" style="margin-top:12px"><button class="pri" id="addf">Ajouter cet arrêt</button></div></div>
<details><summary>Saisie manuelle (identifiants IDFM)</summary>
<label>Identifiant d'arrêt (MonitoringRef, « IDFM:… » ou n° de zone d'arrêt)</label><input id="mref" placeholder="STIF:StopArea:SP:71517:">
<label>Identifiant de ligne (facultatif)</label><input id="mline" placeholder="C01371">
<label>Texte du pictogramme si ligne inconnue (ex. numéro de bus)</label><input id="mbadge" maxlength="7">
<label>Filtre de direction (facultatif, plusieurs avec « | »)</label><input id="mfilter" placeholder="Défense">
<label>Nom affiché</label><input id="mlabel">
<div class="row" style="margin-top:10px"><button id="addm">Ajouter</button></div></details>
</section>

<section><h2>4. Réglages</h2>
<label>Rafraîchissement (secondes)</label><input id="refreshS" type="number" min="10" max="3600">
<label>Durée d'affichage avant veille (secondes)</label><input id="awakeS" type="number" min="10" max="3600">
<label>Veille</label><select id="sleepMode"><option value="0">Veille profonde (sur batterie) — réveil par le bouton BOOT</option>
<option value="1">Écran éteint (sur secteur) — réveil au toucher</option><option value="2">Toujours allumé (sur secteur)</option></select>
<label><input type="checkbox" id="motionWake">Réveil en secouant l'afficheur (expérimental)</label>
<label>Quota d'appels par jour (1000 pour un compte PRIM gratuit)</label><input id="dailyQuota" type="number" min="10" max="60000">
<label>Luminosité</label><input id="brightness" type="range" min="5" max="255">
<label><input type="checkbox" id="flip">Retourner l'écran (180°)</label>
<details><summary>Avancé</summary><div class="row" style="margin-top:10px"><button id="reset">Effacer tous les réglages</button></div></details>
</section>

<div class="foot"><button class="pri" id="save">Enregistrer et redémarrer</button></div>
<div id="toast"></div>
</main><script>
const $=i=>document.getElementById(i);let C={},sel=null;
function h(t,a,...k){const e=document.createElement(t);for(const n in a||{}){if(n.startsWith('on'))e[n]=a[n];else e.setAttribute(n,a[n]);}
for(const c of k.flat())if(c!=null)e.append(c.nodeType?c:document.createTextNode(c));return e}
async function api(u,b){const o=b===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)};
const r=await fetch(u,o);const j=await r.json().catch(()=>({error:'Réponse invalide ('+r.status+')'}));if(!r.ok&&!j.error)j.error='Erreur '+r.status;return j}
function badge(l){const m=l.mode||'bus';const cls=m=='metro'?'':(m=='rer'||m=='train')?'sq':'rect';
return h('span',{class:'badge '+cls,style:`background:${l.color||'#5a6e8c'};color:${l.text||'#fff'}`},l.name||l.badge||l.id||'?')}
async function status(){const s=await api('/api/status');let t=s.sta?`<span class="ok">Connecté à ${s.ssid}</span> · ${s.ip}`:'<span class="err">Pas de connexion Internet</span> (ajoutez un WiFi)';
t+=` · batterie ${s.battery}% · ${s.quotaUsed}/${s.quota} appels aujourd'hui · v${s.version}`;$('st').innerHTML=t;return s}
function renderW(){const L=$('wl');L.innerHTML='';C.wifi.forEach((w,i)=>L.append(h('div',{},h('div',{class:'grow'},w.ssid,h('div',{class:'mut'},w.pass?'nouveau mot de passe':(w.hasPass?'mot de passe enregistré':'réseau ouvert'))),
h('button',{class:'sm',onclick:()=>{C.wifi.splice(i,1);renderW()}},'Retirer'))));if(!C.wifi.length)L.append(h('div',{class:'mut'},'Aucun réseau enregistré.'))}
function renderF(){const L=$('fl');L.innerHTML='';C.favorites.forEach((f,i)=>{const li=f._line||{name:f.badge||(f.line?f.line:'Toutes'),mode:f.line?'bus':'rect'};
L.append(h('div',{},badge(li),h('div',{class:'grow'},f.label||f.stopName||f.stopRef,h('div',{class:'mut'},(f.label?f.stopName+' · ':'')+(f.filter?'vers '+f.filter:'tous les sens'))),
i?h('button',{class:'sm',onclick:()=>{C.favorites.splice(i-1,0,C.favorites.splice(i,1)[0]);renderF()}},'↑'):null,
h('button',{class:'sm',onclick:()=>{C.favorites.splice(i,1);renderF()}},'Retirer')))});
if(!C.favorites.length)L.append(h('div',{class:'mut'},'Aucun arrêt pour l\'instant.'))}
async function load(){C=await api('/api/config');C.favorites=C.favorites||[];C.wifi=C.wifi||[];
$('key').placeholder=C.apiKeySet?'Clé enregistrée ('+(C.apiKeyHint||'')+') — laisser vide pour la garder':'Collez votre clé ici';
for(const k of['refreshS','awakeS','dailyQuota','brightness','sleepMode'])$(k).value=C[k];$('motionWake').checked=C.motionWake;$('flip').checked=C.flip;
renderW();renderF();status()}
$('scan').onclick=async()=>{$('scan').disabled=true;$('scanres').innerHTML='<div class="mut">Recherche…</div>';const r=await api('/api/scan');$('scan').disabled=false;
const L=$('scanres');L.innerHTML='';(r.networks||[]).forEach(n=>L.append(h('div',{class:'pick',onclick:()=>{$('nssid').value=n.ssid;$('npass').focus()}},h('div',{class:'grow'},n.ssid),h('span',{class:'mut'},(n.secure?'🔒 ':'')+n.rssi+' dBm'))))};
$('addw').onclick=async()=>{const s=$('nssid').value.trim(),p=$('npass').value;if(!s)return;C.wifi=C.wifi.filter(w=>w.ssid!=s);if(C.wifi.length>=3)C.wifi.pop();
C.wifi.unshift({ssid:s,pass:p,hasPass:!!p});renderW();$('wmsg').textContent='Connexion à '+s+'…';await api('/api/connect',{ssid:s,pass:p});
for(let i=0;i<20;i++){await new Promise(r=>setTimeout(r,1000));const st=await status().catch(()=>({}));if(st.sta&&st.ssid==s){$('wmsg').innerHTML='<span class="ok">Connecté ✓</span> — vous pouvez chercher vos arrêts.';return}}
$('wmsg').innerHTML='<span class="err">Connexion impossible : vérifiez le mot de passe.</span>'};
$('go').onclick=async()=>{const q=$('q').value.trim();if(!q)return;$('smsg').textContent='Recherche…';$('sres').innerHTML='';$('lsel').style.display='none';
const r=await api('/api/search?q='+encodeURIComponent(q));if(r.error){$('smsg').innerHTML='<span class="err">'+r.error+'</span>';return}
$('smsg').textContent=r.stops.length?'Choisissez l\'arrêt :':'Aucun résultat.';const icon={metro:'Ⓜ️',rail:'🚆',tram:'🚊',bus:'🚌'};
r.stops.forEach(s=>$('sres').append(h('div',{class:'pick',onclick:()=>pickStop(s)},h('span',{},icon[s.type]||'📍'),h('div',{class:'grow'},s.name,h('div',{class:'mut'},s.town)))))};
$('q').onkeydown=e=>{if(e.key=='Enter')$('go').click()};
async function pickStop(s){$('sres').innerHTML='';$('smsg').textContent='Lignes desservies à '+s.name+'…';
const r=await api('/api/lines?ref='+encodeURIComponent(s.ref)+'&key='+encodeURIComponent($('key').value.trim()));
if(r.error){$('smsg').innerHTML='<span class="err">'+r.error+'</span>';return}
if(!r.lines||!r.lines.length){$('smsg').textContent='Aucun passage annoncé en ce moment à cet arrêt (réessayez en journée).';return}
$('smsg').textContent='';sel={stopRef:r.ref,stopName:r.stopName||s.name,line:'',filter:'',_line:null};$('ltitle').textContent='Ligne à '+sel.stopName+' :';
const LC=$('lchips');LC.innerHTML='';r.lines.forEach(l=>{const c=h('span',{class:'chip',onclick:()=>{[...LC.children].forEach(x=>x.classList.remove('on'));c.classList.add('on');sel.line=l.id;sel._line=l;sel.filter='';
const D=$('dchips');D.innerHTML='';const all=h('span',{class:'chip on',onclick:()=>{[...D.children].forEach(x=>x.classList.remove('on'));all.classList.add('on');sel.filter=''}},'Tous les sens');D.append(all);
l.dests.forEach(d=>{const dc=h('span',{class:'chip',onclick:()=>{[...D.children].forEach(x=>x.classList.remove('on'));dc.classList.add('on');sel.filter=d}},'→ '+d);D.append(dc)})}},badge(l),' ',l.dests.slice(0,2).join(' / '));LC.append(c)});
$('lsel').style.display='';LC.children[0].click()}
$('addf').onclick=()=>{if(!sel)return;if(C.favorites.length>=4){alert('4 arrêts maximum');return}
C.favorites.push({stopRef:sel.stopRef,stopName:sel.stopName,line:sel.line,badge:'',filter:sel.filter,label:$('flabel').value.trim(),_line:sel._line});
renderF();$('lsel').style.display='none';$('flabel').value='';$('q').value=''};
$('addm').onclick=()=>{const r=$('mref').value.trim();if(!r)return;if(C.favorites.length>=4){alert('4 arrêts maximum');return}
C.favorites.push({stopRef:r,stopName:$('mlabel').value.trim()||r,line:$('mline').value.trim(),badge:$('mbadge').value.trim(),filter:$('mfilter').value.trim(),label:$('mlabel').value.trim()});renderF()};
$('save').onclick=async()=>{const b={wifi:C.wifi.map(w=>({ssid:w.ssid,pass:w.pass||''})),favorites:C.favorites.map(({_line,...f})=>f),
refreshS:+$('refreshS').value,awakeS:+$('awakeS').value,dailyQuota:+$('dailyQuota').value,brightness:+$('brightness').value,sleepMode:+$('sleepMode').value,
motionWake:$('motionWake').checked,flip:$('flip').checked};const k=$('key').value.trim();if(k)b.apiKey=k;
const r=await api('/api/config',b);if(r.error){$('toast').innerHTML='<span class="err">'+r.error+'</span>';return}
$('toast').innerHTML='<span class="ok">Enregistré ✓</span> '+(r.ready?'L\'afficheur redémarre…':'<span class="err">Il manque encore : '+r.missing+'</span>');if(r.ready)api('/api/reboot',{})};
$('reset').onclick=async()=>{if(confirm('Effacer WiFi, clé et arrêts ?')){await api('/api/reset',{});$('toast').textContent='Réglages effacés, redémarrage…'}};
load();setInterval(()=>status().catch(()=>{}),15000);
</script></body></html>)HTML";
