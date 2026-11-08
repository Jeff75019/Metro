// Page web de configuration, servie par l'afficheur (http://192.168.4.1).
// Aperçu sur PC sans la carte : python3 tools/portal_preview.py
#pragma once

static const char PORTAL_HTML[] = R"HTML(<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0a1020">
<title>Afficheur Métro</title>
<style>
:root{--bg:#0a1020;--card:#111a2e;--card2:#0d1527;--line:#1d2843;--txt:#f4f6fb;--mut:#8c98b4;--faint:#55617f;
--acc:#ffc12e;--ok:#34d399;--err:#ff6b6b;--warn:#ffa94d;--r:16px}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html{background:var(--bg)}body{margin:0;color:var(--txt);font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,system-ui,sans-serif}
main{max-width:560px;margin:0 auto;padding:20px 16px 110px}
.top{display:flex;align-items:center;gap:12px;margin:4px 0 6px}
.logo{width:44px;height:44px;border-radius:50%;background:var(--txt);color:var(--bg);display:grid;place-items:center;font:800 24px/1 system-ui;flex:none}
h1{font-size:21px;margin:0;letter-spacing:-.2px}.sub{color:var(--mut);font-size:13px}
.stripe{display:flex;height:3px;border-radius:2px;overflow:hidden;margin:14px 0 4px}.stripe i{flex:1}
.status{display:flex;flex-wrap:wrap;gap:6px;margin:12px 0 4px}
.tag{display:inline-flex;align-items:center;gap:6px;background:var(--card);border:1px solid var(--line);border-radius:999px;padding:5px 10px;font-size:12.5px;color:var(--mut)}
.tag b{font-weight:600;color:var(--txt)}.dot{width:7px;height:7px;border-radius:50%;background:var(--faint)}.dot.ok{background:var(--ok)}.dot.err{background:var(--err)}
section{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:18px 16px;margin:14px 0}
.sh{display:flex;align-items:center;gap:10px;margin-bottom:4px}
.num{width:26px;height:26px;border-radius:50%;background:rgba(255,193,46,.14);color:var(--acc);display:grid;place-items:center;font-weight:700;font-size:13px;flex:none}
.num.done{background:rgba(52,211,153,.15);color:var(--ok)}
h2{font-size:16.5px;margin:0}.desc{color:var(--mut);font-size:13.5px;margin:2px 0 12px 36px}
label.f{display:block;font-size:12.5px;color:var(--mut);margin:12px 0 6px;font-weight:500}
label.chk{display:flex;align-items:center;gap:8px;margin:10px 0 0;font-size:13.5px;color:var(--mut)}label.chk input{width:18px;height:18px;margin:0;padding:0;accent-color:var(--acc)}
input,select{width:100%;padding:12px 14px;border-radius:12px;border:1px solid var(--line);background:var(--card2);color:var(--txt);font-size:16px;outline:none;transition:border-color .15s,box-shadow .15s}
input:focus,select:focus{border-color:var(--acc);box-shadow:0 0 0 3px rgba(255,193,46,.18)}
input::placeholder{color:var(--faint)}
button{border:0;border-radius:12px;padding:12px 16px;font-size:15px;font-weight:600;background:var(--line);color:var(--txt);cursor:pointer;transition:filter .15s,transform .05s}
button:active{transform:scale(.98)}button:hover{filter:brightness(1.15)}button:disabled{opacity:.45}
button.pri{background:var(--acc);color:#1a1300}button.ghost{background:transparent;color:var(--mut);padding:8px 10px}
button.icon{background:transparent;color:var(--mut);padding:8px;border-radius:10px;line-height:0}button.icon:hover{background:var(--line)}
.row{display:flex;gap:8px;align-items:center}.row>input{flex:1}.row>button{flex:none}
.list{margin:4px 0}.item{display:flex;align-items:center;gap:12px;padding:11px 4px;border-top:1px solid var(--line)}.item:first-child{border-top:0}
.pick{cursor:pointer;border-radius:12px;padding:11px 10px}.pick:hover{background:var(--card2)}
.grow{flex:1;min-width:0}.name{font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.meta{color:var(--mut);font-size:12.5px}
.empty{color:var(--faint);font-size:13.5px;padding:8px 4px}
.badge{display:inline-flex;align-items:center;justify-content:center;min-width:28px;height:28px;padding:0 7px;border-radius:14px;font-weight:700;font-size:14px;flex:none}
.badge.sq{border-radius:7px}.badge.rect{border-radius:5px}
.mode{display:inline-flex;align-items:center;justify-content:center;height:22px;min-width:22px;padding:0 5px;border-radius:11px;background:var(--txt);color:var(--bg);font-weight:800;font-size:11px;flex:none}
.mode.r{border-radius:5px}
.pics{display:flex;align-items:center;gap:5px;flex:none}
.chips{display:flex;flex-wrap:wrap;gap:8px;margin-top:6px}
.chip{display:inline-flex;align-items:center;gap:8px;border:1px solid var(--line);border-radius:999px;padding:6px 12px 6px 6px;cursor:pointer;font-size:13.5px;background:var(--card2);color:var(--txt)}
.chip.t{padding:7px 13px}.chip.on{border-color:var(--acc);background:rgba(255,193,46,.08)}
.msg{font-size:13.5px;margin-top:8px;color:var(--mut)}.msg.ok{color:var(--ok)}.msg.err{color:var(--err)}
.panel{background:var(--card2);border:1px solid var(--line);border-radius:14px;padding:14px;margin-top:12px}
.set{display:flex;align-items:center;justify-content:space-between;gap:16px;padding:12px 0;border-top:1px solid var(--line)}.set:first-of-type{border-top:0}
.set .l{flex:1}.set .l b{display:block;font-weight:600;font-size:14.5px}.set .l span{color:var(--mut);font-size:12.5px}
.set input[type=number]{width:96px;text-align:right}
.seg{display:flex;background:var(--card2);border:1px solid var(--line);border-radius:12px;padding:3px;gap:3px;margin-top:8px}
.seg button{flex:1;background:transparent;color:var(--mut);padding:9px 6px;font-size:13px;border-radius:9px}.seg button.on{background:var(--line);color:var(--txt)}
.sw{position:relative;width:46px;height:28px;flex:none}.sw input{opacity:0;width:0;height:0;position:absolute}
.sw span{position:absolute;inset:0;background:var(--line);border-radius:14px;transition:.2s;cursor:pointer}
.sw span:before{content:"";position:absolute;width:22px;height:22px;left:3px;top:3px;border-radius:50%;background:var(--mut);transition:.2s}
.sw input:checked+span{background:rgba(255,193,46,.35)}.sw input:checked+span:before{transform:translateX(18px);background:var(--acc)}
input[type=range]{padding:0;height:28px;background:transparent;border:0;accent-color:var(--acc);box-shadow:none}
details{margin-top:14px}summary{cursor:pointer;color:var(--mut);font-size:13.5px;list-style:none}summary:before{content:"▸ "}details[open] summary:before{content:"▾ "}
a{color:var(--acc);text-decoration:none}a:hover{text-decoration:underline}
.bar{position:fixed;left:0;right:0;bottom:0;padding:12px 16px calc(12px + env(safe-area-inset-bottom));background:linear-gradient(transparent,var(--bg) 30%);display:flex;justify-content:center}
.bar .in{max-width:560px;width:100%;display:flex;flex-direction:column;gap:8px}.bar button{width:100%;padding:15px;font-size:16px;box-shadow:0 8px 24px rgba(0,0,0,.35)}
#toast{text-align:center;font-size:13.5px;min-height:18px}
svg{display:block}
</style></head><body><main>
<div class="top"><div class="logo">M</div><div><h1>Afficheur Métro</h1><div class="sub">Prochains passages en temps réel · Île-de-France</div></div></div>
<div class="stripe" id="stripe"></div>
<div class="status" id="st"><span class="tag"><span class="dot"></span>Connexion à l'afficheur…</span></div>

<section><div class="sh"><span class="num" id="n1">1</span><h2>Réseau WiFi</h2></div>
<div class="desc">Celui que l'afficheur utilisera pour récupérer les horaires. Jusqu'à 3 : maison, bureau, partage de connexion…</div>
<div class="list" id="wl"></div>
<div class="panel">
<div class="row"><button id="scan" style="width:100%">Rechercher les réseaux à proximité</button></div>
<div class="list" id="scanres"></div>
<label class="f">Nom du réseau</label><input id="nssid" autocomplete="off" autocapitalize="off" autocorrect="off" spellcheck="false" placeholder="ex. Freebox-1A2B3C">
<label class="f">Mot de passe</label><input id="npass" type="password" autocomplete="off" autocapitalize="off" autocorrect="off" spellcheck="false" placeholder="••••••••">
<label class="chk"><input type="checkbox" id="showp"> Afficher le mot de passe</label>
<div style="margin-top:12px"><button class="pri" id="addw" style="width:100%">Ajouter et se connecter</button></div>
<div id="wmsg" class="msg"></div></div>
</section>

<section><div class="sh"><span class="num" id="n2">2</span><h2>Clé API PRIM <span class="meta">· métro, RER, tram, bus</span></h2></div>
<div class="desc">Gratuite : créez un compte sur <a href="https://prim.iledefrance-mobilites.fr" target="_blank">prim.iledefrance-mobilites.fr</a>, abonnez-vous à l'API « Prochains passages (requête unitaire) », puis générez un jeton dans « Mon compte ».</div>
<input id="key" type="password" autocomplete="off">
</section>

<section><div class="sh"><span class="num" id="n3">3</span><h2>Mes arrêts</h2></div>
<div class="desc">Facultatif : sur l'afficheur, vous pouvez aussi choisir n'importe quelle ligne et station (accueil → Métro, RER ou Tram). Les arrêts ajoutés ici forment « Mes arrêts » : métro, RER, tram ou bus, 4 au maximum, passez de l'un à l'autre en glissant le doigt vers la gauche ou la droite.</div>
<div class="list" id="fl"></div>
<div class="panel">
<div class="row"><input id="q" placeholder="Station ou arrêt, ex. Bastille" autocomplete="off"><button class="pri" id="go" aria-label="Chercher"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/></svg></button></div>
<div id="smsg" class="msg"></div><div class="list" id="sres"></div>
<div id="lsel" style="display:none"><label class="f" id="ltitle"></label><div class="chips" id="lchips"></div>
<label class="f">Destinations à afficher (plusieurs possibles, ex. toutes celles d'un même sens)</label><div class="chips" id="dchips"></div>
<label class="f">Nom affiché (facultatif)</label><input id="flabel" placeholder="ex. Maison → Bureau">
<div style="margin-top:12px"><button class="pri" id="addf" style="width:100%">Ajouter cet arrêt</button></div></div>
</div>
<details><summary>Saisie manuelle des identifiants IDFM</summary><div class="panel">
<label class="f">Identifiant d'arrêt (MonitoringRef, « IDFM:… » ou n° de zone d'arrêt)</label><input id="mref" placeholder="STIF:StopArea:SP:71517:">
<label class="f">Identifiant de ligne (facultatif)</label><input id="mline" placeholder="C01371">
<label class="f">Texte du pictogramme si la ligne est inconnue</label><input id="mbadge" maxlength="7" placeholder="ex. 91">
<label class="f">Filtre de direction (plusieurs avec « | »)</label><input id="mfilter" placeholder="Défense">
<label class="f">Nom affiché</label><input id="mlabel">
<div style="margin-top:12px"><button id="addm" style="width:100%">Ajouter</button></div></div></details>
</section>

<section><div class="sh"><span class="num" id="n4">4</span><h2>Trains grandes lignes <span class="meta">· facultatif</span></h2></div>
<div class="desc">Départs et arrivées des gares SNCF (TGV, Intercités, TER…), avec les voies. Sur l'afficheur, « Trains SNCF » permet de choisir n'importe quelle grande gare ; les gares cochées ici forment « Mes gares ».</div>
<label class="f">Source des données</label>
<div class="seg" id="srcSeg"><button data-v="trainnomade">Train Nomade</button><button data-v="sncf">API SNCF (clé)</button></div>
<div class="msg" id="srcHelp"></div>
<div id="skeyBox"><label class="f">Clé API SNCF</label><input id="skey" type="password" autocomplete="off"></div>
<label class="f">Gares de Paris</label><div class="list" id="pl"></div>
<label class="f" id="otherLbl">Autres gares</label><div class="list" id="ol"></div>
<div class="panel"><div class="row"><input id="sq" placeholder="Autre gare, ex. Marne-la-Vallée" autocomplete="off"><button class="pri" id="sgo" aria-label="Chercher"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/></svg></button></div>
<div id="smsg2" class="msg"></div><div class="list" id="sres2"></div></div>
<div class="set" style="margin-top:6px"><div class="l"><b>Grandes lignes uniquement</b><span>masquer RER, Transilien et autocars</span></div><label class="sw"><input type="checkbox" id="mainlineOnly"><span></span></label></div>
</section>

<section><div class="sh"><span class="num">5</span><h2>Réglages</h2></div>
<div class="set"><div class="l"><b>Mise en veille</b><span id="sleepHelp"></span></div></div>
<div class="seg" id="sleepSeg"><button data-v="0">Batterie</button><button data-v="1">Écran éteint</button><button data-v="2">Permanent</button></div>
<div class="set" style="margin-top:8px"><div class="l"><b>Durée d'affichage</b><span>avant la mise en veille, en secondes</span></div><input id="awakeS" type="number" min="10" max="3600"></div>
<div class="set"><div class="l"><b>Rafraîchissement</b><span>intervalle entre deux mises à jour, en secondes</span></div><input id="refreshS" type="number" min="10" max="3600"></div>
<div class="set"><div class="l"><b>Quota d'appels par jour</b><span>1000 pour un compte PRIM gratuit</span></div><input id="dailyQuota" type="number" min="10" max="60000"></div>
<div class="set"><div class="l"><b>Réveil en secouant</b><span>expérimental</span></div><label class="sw"><input type="checkbox" id="motionWake"><span></span></label></div>
<div class="set"><div class="l"><b>Retourner l'écran</b><span>rotation de 180°</span></div><label class="sw"><input type="checkbox" id="flip"><span></span></label></div>
<div class="set" style="display:block"><div class="l"><b>Luminosité</b></div><input id="brightness" type="range" min="5" max="255"></div>
<details><summary>Avancé</summary><div style="margin-top:10px"><button class="ghost" id="reset" style="color:var(--err)">Effacer tous les réglages</button></div></details>
</section>
</main>
<div class="bar"><div class="in"><div id="toast"></div><button class="pri" id="save">Enregistrer et redémarrer</button></div></div>
<script>
const $=i=>document.getElementById(i);let C={},sel=null;
const COLORS=['#ffbe00','#0055c8','#6e6e00','#82c8e6','#a0006e','#ff5a00','#82dc73','#ff82b4','#d282be','#d2d200','#dc9600','#6e491e','#00643c','#640082'];
$('stripe').innerHTML=COLORS.map(c=>`<i style="background:${c}"></i>`).join('');
const ICON={x:'<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><path d="M6 6l12 12M18 6 6 18"/></svg>',
up:'<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 14 6-6 6 6"/></svg>',
lock:'<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/></svg>'};
function wifiIcon(rssi){const n=rssi>-60?3:rssi>-72?2:1;return `<svg width="20" height="16" viewBox="0 0 20 16">${[0,1,2].map(i=>`<rect x="${2+i*6}" y="${11-i*4}" width="4" height="${5+i*4}" rx="1.5" fill="${i<n?'#f4f6fb':'#1d2843'}"/>`).join('')}</svg>`}
function h(t,a,...k){const e=document.createElement(t);for(const n in a||{}){if(n.startsWith('on'))e[n]=a[n];else if(n=='html')e.innerHTML=a[n];else e.setAttribute(n,a[n]);}
for(const c of k.flat())if(c!=null)e.append(c.nodeType?c:document.createTextNode(c));return e}
async function api(u,b){const o=b===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)};
const ac=new AbortController();const t=setTimeout(()=>ac.abort(),45000);o.signal=ac.signal;
try{const r=await fetch(u,o);const j=await r.json().catch(()=>({error:'Réponse invalide ('+r.status+')'}));if(!r.ok&&!j.error)j.error='Erreur '+r.status;return j}finally{clearTimeout(t)}}
const LOST=()=>({error:'L\'afficheur ne répond pas : vérifiez que ce téléphone est toujours connecté au réseau « '+(AP||'Metro-…')+' » (ou au même WiFi que l\'afficheur), puis réessayez.'});
function msg(id,t,cls){const e=$(id);e.className='msg'+(cls?' '+cls:'');e.textContent=t||''}
const MODE={metro:['M',''],rer:['RER','r'],train:['TRAIN','r'],tram:['T',''],navette:['VAL','r'],bus:['BUS','r']};
function modePic(m){const d=MODE[m]||MODE.bus;return h('span',{class:'mode '+d[1]},d[0])}
function badge(l){const m=l.mode||'bus';const cls=m=='metro'?'':(m=='rer'||m=='train')?'sq':'rect';
return h('span',{class:'badge '+cls,style:`background:${l.color||'#5a6e8c'};color:${l.text||'#fff'}`},l.name||l.badge||l.id||'?')}
function pics(l){return h('span',{class:'pics'},modePic(l.mode),badge(l))}
let AP='';
async function status(){const s=await api('/api/status');AP=s.ap||AP;const T=$('st');T.innerHTML='';
T.append(h('span',{class:'tag'},h('span',{class:'dot '+(s.sta?'ok':'err')}),s.sta?h('span',{},'Internet via ',h('b',{},s.ssid)):'Pas encore d\'accès Internet'));
T.append(h('span',{class:'tag'},'Batterie ',h('b',{},s.battery+' %')));
T.append(h('span',{class:'tag'},'Appels PRIM ',h('b',{},s.quotaUsed+' / '+s.quota)));
if(C.sncfKeySet||s.sncfUsed)T.append(h('span',{class:'tag'},'Appels SNCF ',h('b',{},s.sncfUsed+' / '+s.sncfQuota)));
T.append(h('span',{class:'tag'},'v'+s.version));if(s.psram===0)T.append(h('span',{class:'tag',style:'color:var(--err);border-color:var(--err)'},'PSRAM désactivée : Outils → PSRAM → OPI PSRAM, puis téléverser'));$('n1').classList.toggle('done',!!s.sta);return s}
function renderW(){const L=$('wl');L.innerHTML='';C.wifi.forEach((w,i)=>L.append(h('div',{class:'item'},h('span',{html:wifiIcon(-50)}),h('div',{class:'grow'},h('div',{class:'name'},w.ssid),h('div',{class:'meta'},w.pass?'Nouveau mot de passe':(w.hasPass?'Mot de passe enregistré':'Réseau ouvert'))),
h('button',{class:'icon',title:'Retirer',html:ICON.x,onclick:()=>{C.wifi.splice(i,1);renderW()}}))));if(!C.wifi.length)L.append(h('div',{class:'empty'},'Aucun réseau enregistré pour l\'instant.'))}
function renderF(){const L=$('fl');L.innerHTML='';C.favorites.forEach((f,i)=>{const li=f._line||{name:f.badge||f.line||'Tout',mode:'bus'};
L.append(h('div',{class:'item'},pics(li),h('div',{class:'grow'},h('div',{class:'name'},f.label||f.stopName||f.stopRef),h('div',{class:'meta'},(f.label?f.stopName+' · ':'')+(f.filter?'Vers '+f.filter.split('|').join(', '):'Toutes les destinations'))),
i?h('button',{class:'icon',title:'Monter',html:ICON.up,onclick:()=>{C.favorites.splice(i-1,0,C.favorites.splice(i,1)[0]);renderF()}}):null,
h('button',{class:'icon',title:'Retirer',html:ICON.x,onclick:()=>{C.favorites.splice(i,1);renderF()}})))});
if(!C.favorites.length)L.append(h('div',{class:'empty'},'Aucun arrêt pour l\'instant : cherchez-en un ci-dessous.'));$('n3').classList.toggle('done',C.favorites.length>0)}
const PARIS=[["87686006,87686030","Gare de Lyon"],["87271007,87271031,87271023","Gare du Nord"],["87113001","Gare de l'Est"],["87391003,87391102,87391011","Montparnasse"],["87384008","Saint-Lazare"],["87547000,87547026","Austerlitz"],["87686667","Bercy"]];
const uic1=id=>(id.match(/\d{8}/)||[''])[0];const sameSt=(a,b)=>uic1(a)==uic1(b);
const SRC={trainnomade:'Données SNCF (ODbL) diffusées par <a href="https://trainnomade.fr/api" target="_blank">Train Nomade</a> : gratuit, sans clé, mises à jour toutes les 2 minutes. Service indépendant de la SNCF.',
sncf:'API officielle de la SNCF : clé gratuite (5 000 requêtes par jour) sur <a href="https://numerique.sncf.com/startup/api/token-developpeur/" target="_blank">numerique.sncf.com</a>. Pas de numéro de voie.'};
function setSrc(v){C.trainSource=v;[...$('srcSeg').children].forEach(b=>b.classList.toggle('on',b.dataset.v==v));$('srcHelp').innerHTML=SRC[v];$('skeyBox').style.display=v=='sncf'?'':'none';renderS()}
[...$('srcSeg').children].forEach(b=>b.onclick=()=>setSrc(b.dataset.v));
function renderS(){const L=$('pl');L.innerHTML='';PARIS.forEach(([id,name])=>{const cb=h('input',{type:'checkbox'});cb.checked=C.stations.some(s=>sameSt(s.id,id));
cb.onchange=()=>{if(cb.checked){if(C.stations.length>=10){cb.checked=false;return}C.stations.push({id,name})}else C.stations=C.stations.filter(s=>!sameSt(s.id,id));renderS()};
L.append(h('div',{class:'item'},modePic('train'),h('div',{class:'grow name'},name),h('label',{class:'sw'},cb,h('span'))))});
const O=$('ol');O.innerHTML='';const others=C.stations.filter(s=>!PARIS.some(p=>sameSt(p[0],s.id)));$('otherLbl').style.display=others.length?'':'none';
others.forEach(s=>O.append(h('div',{class:'item'},modePic('train'),h('div',{class:'grow name'},s.name),h('button',{class:'icon',title:'Retirer',html:ICON.x,onclick:()=>{C.stations=C.stations.filter(x=>x.id!=s.id);renderS()}}))));
$('n4').classList.toggle('done',C.stations.length>0&&(C.trainSource!='sncf'||!!C.sncfKeySet))}
$('sgo').onclick=async()=>{const q=$('sq').value.trim();if(!q)return;msg('smsg2','Recherche…');$('sres2').innerHTML='';
const r=await api('/api/sncf/search?q='+encodeURIComponent(q)+'&src='+C.trainSource+'&key='+encodeURIComponent($('skey').value.trim()));if(r.error){msg('smsg2',r.error,'err');return}
msg('smsg2',r.stations.length?'Touchez une gare pour l\'ajouter :':'Aucune gare trouvée.');
r.stations.forEach(st=>$('sres2').append(h('div',{class:'item pick',onclick:()=>{if(!C.stations.some(x=>sameSt(x.id,st.id))){if(C.stations.length>=10){msg('smsg2','10 gares au maximum.','err');return}C.stations.push(st)}
renderS();$('sres2').innerHTML='';$('sq').value='';msg('smsg2','Gare ajoutée ✓','ok')}},modePic('train'),h('div',{class:'grow name'},st.name))))};
$('sq').onkeydown=e=>{if(e.key=='Enter')$('sgo').click()};
const SLEEP=['Veille profonde après la durée d\'affichage ; réveil par le bouton BOOT.','Sur secteur : l\'écran s\'éteint, un toucher le rallume.','Sur secteur, écran toujours allumé ; rafraîchissement étalé pour respecter le quota.'];
function setSleep(v){C.sleepMode=+v;[...$('sleepSeg').children].forEach(b=>b.classList.toggle('on',b.dataset.v==v));$('sleepHelp').textContent=SLEEP[v]}
[...$('sleepSeg').children].forEach(b=>b.onclick=()=>setSleep(b.dataset.v));
async function load(){C=await api('/api/config');C.favorites=C.favorites||[];C.wifi=C.wifi||[];
C.stations=C.stations||[];$('skey').placeholder=C.sncfKeySet?'Enregistrée '+(C.sncfKeyHint||'')+' (vide = inchangée)':'Collez votre clé SNCF ici';$('mainlineOnly').checked=C.mainlineOnly!==false;setSrc(C.trainSource||'trainnomade');
$('key').placeholder=C.apiKeySet?'Enregistrée '+(C.apiKeyHint||'')+' (vide = inchangée)':'Collez votre jeton ici';$('n2').classList.toggle('done',!!C.apiKeySet);
for(const k of['refreshS','awakeS','dailyQuota','brightness'])$(k).value=C[k];$('motionWake').checked=C.motionWake;$('flip').checked=C.flip;setSleep(C.sleepMode||0);
renderW();renderF();status().catch(()=>{})}
$('showp').onchange=()=>{$('npass').type=$('showp').checked?'text':'password'};
$('scan').onclick=async()=>{$('scan').disabled=true;$('scan').textContent='Recherche en cours…';const r=await api('/api/scan').catch(()=>({error:'L\'afficheur ne répond pas.'}));$('scan').disabled=false;$('scan').textContent='Rechercher à nouveau';
const L=$('scanres');L.innerHTML='';if(r.error){msg('wmsg',r.error,'err');return}msg('wmsg','');(r.networks||[]).forEach(n=>L.append(h('div',{class:'item pick',onclick:()=>{$('nssid').value=n.ssid;$('npass').focus()}},h('span',{html:wifiIcon(n.rssi)}),h('div',{class:'grow name'},n.ssid),n.secure?h('span',{class:'meta',html:ICON.lock}):null)));
L.append(h('div',{class:'empty'},'Votre réseau n\'apparaît pas ? L\'afficheur ne capte que le WiFi 2,4 GHz (partage de connexion d\'iPhone : activez « Maximiser la compatibilité »).'))};
$('addw').onclick=async()=>{const s=$('nssid').value.trim(),p=$('npass').value;if(!s)return;$('addw').disabled=true;
try{msg('wmsg','Connexion à « '+s+' »…');const r=await api('/api/connect',{ssid:s,pass:p}).catch(()=>({error:'L\'afficheur ne répond pas.'}));if(r.error){msg('wmsg',r.error,'err');return}
C.wifi=C.wifi.filter(w=>w.ssid!=s);if(C.wifi.length>=3)C.wifi.pop();C.wifi.unshift({ssid:s,pass:p,hasPass:!!p});renderW();
let lost=0;for(let i=0;i<60;i++){await new Promise(r=>setTimeout(r,1000));const st=await status().catch(()=>null);
if(!st){if(++lost>=2)msg('wmsg','Liaison avec l\'afficheur coupée : c\'est normal s\'il change de canal pour rejoindre « '+s+' ». Si besoin, reconnectez ce téléphone au réseau « '+(AP||'Metro-…')+' » : la page se mettra à jour. Le WiFi est enregistré dès que la connexion réussit (l\'écran de l\'afficheur l\'indique).');continue}
lost=0;if(st.sta&&st.ssid==s){msg('wmsg','Connecté et enregistré ✓ Vous pouvez chercher vos arrêts.','ok');return}
if(st.staState=='failed'&&st.staTarget==s){msg('wmsg','Connexion impossible : '+st.staError+'. '+(st.staHint||''),'err');return}}
msg('wmsg','Pas de réponse : regardez l\'écran de l\'afficheur, il indique le résultat de la connexion.','err')}finally{$('addw').disabled=false}};
let SQ=0,STO;
async function searchStops(){clearTimeout(STO);const q=$('q').value.trim();if(q.length<2){msg('smsg','Tapez au moins 2 lettres.');return}
const n=++SQ;msg('smsg','Recherche de « '+q+' » par l\'afficheur… (quelques secondes)');$('sres').innerHTML='';$('lsel').style.display='none';
const r=await api('/api/search?q='+encodeURIComponent(q)).catch(LOST);if(n!=SQ)return;if(r.error){msg('smsg',r.error,'err');return}
msg('smsg',r.stops.length?'Touchez votre arrêt :':'Aucun arrêt trouvé pour « '+q+' ». Essayez un autre nom (ex. « Gare de Lyon », « Nation »).');
r.stops.forEach(s=>$('sres').append(h('div',{class:'item pick',onclick:()=>pickStop(s)},modePic(s.type=='rail'?'rer':s.type),h('div',{class:'grow'},h('div',{class:'name'},s.name),h('div',{class:'meta'},s.town)))))};
$('go').onclick=searchStops;$('q').onkeydown=e=>{if(e.key=='Enter')searchStops()};
$('q').oninput=()=>{clearTimeout(STO);if($('q').value.trim().length>=3)STO=setTimeout(searchStops,1000)};
function choose(box,el){[...box.children].forEach(x=>x.classList.remove('on'));el.classList.add('on')}
async function pickStop(s){$('sres').innerHTML='';msg('smsg','Lignes desservies à '+s.name+' (interrogation de PRIM)…');
const r=await api('/api/lines?ref='+encodeURIComponent(s.ref)+'&key='+encodeURIComponent($('key').value.trim())).catch(LOST);
if(r.error){msg('smsg',r.error,'err');return}
if(!r.lines||!r.lines.length){msg('smsg','Aucun passage annoncé en ce moment à cet arrêt (réessayez en journée).');return}
msg('smsg','');sel={stopRef:r.ref,stopName:r.stopName||s.name,line:'',filter:'',_line:null};$('ltitle').textContent='Ligne à '+sel.stopName;
const LC=$('lchips');LC.innerHTML='';r.lines.forEach(l=>{const c=h('span',{class:'chip',onclick:()=>{choose(LC,c);sel.line=l.id;sel._line=l;sel.filter='';
const D=$('dchips');D.innerHTML='';const picked=new Set();
const all=h('span',{class:'chip t on',onclick:()=>{picked.clear();[...D.children].forEach(x=>x.classList.remove('on'));all.classList.add('on');sel.filter=''}},'Toutes');D.append(all);
l.dests.forEach(d=>{const dc=h('span',{class:'chip t',onclick:()=>{if(picked.has(d)){picked.delete(d);dc.classList.remove('on')}else{picked.add(d);dc.classList.add('on')}
all.classList.toggle('on',!picked.size);sel.filter=[...picked].join('|')}},'→ '+d);D.append(dc)})}},badge(l),l.dests.slice(0,2).join(' / '));LC.append(c)});
$('lsel').style.display='';LC.children[0].click()}
$('addf').onclick=async()=>{if(!sel)return;if(C.favorites.length>=4){msg('smsg','4 arrêts au maximum.','err');return}
const L=sel._line||{};C.favorites.push({stopRef:sel.stopRef,stopName:sel.stopName,line:sel.line,badge:(L.name||'').slice(0,7),color:L.color,text:L.text,mode:L.mode,
filter:sel.filter,label:$('flabel').value.trim(),_line:sel._line});
renderF();$('lsel').style.display='none';$('flabel').value='';$('q').value='';msg('smsg','Enregistrement…');
const r=await saveCfg().catch(LOST);msg('smsg',r.error?'Arrêt ajouté, mais pas encore enregistré ('+r.error+') : touchez « Enregistrer » en bas de la page.':'Arrêt ajouté et enregistré ✓',r.error?'err':'ok')};
$('addm').onclick=()=>{const r=$('mref').value.trim();if(!r)return;if(C.favorites.length>=4){alert('4 arrêts au maximum');return}
C.favorites.push({stopRef:r,stopName:$('mlabel').value.trim()||r,line:$('mline').value.trim(),badge:$('mbadge').value.trim(),filter:$('mfilter').value.trim(),label:$('mlabel').value.trim()});renderF()};
function cfgBody(){const b={wifi:C.wifi.map(w=>({ssid:w.ssid,pass:w.pass||''})),favorites:C.favorites.map(({_line,...f})=>f),
refreshS:+$('refreshS').value,awakeS:+$('awakeS').value,dailyQuota:+$('dailyQuota').value,brightness:+$('brightness').value,sleepMode:C.sleepMode,
motionWake:$('motionWake').checked,flip:$('flip').checked,stations:C.stations,mainlineOnly:$('mainlineOnly').checked,trainSource:C.trainSource};
const k=$('key').value.trim();if(k)b.apiKey=k;const sk=$('skey').value.trim();if(sk)b.sncfKey=sk;return b}
const saveCfg=()=>api('/api/config',cfgBody());
$('save').onclick=async()=>{const r=await saveCfg().catch(LOST);const T=$('toast');if(r.error){T.className='msg err';T.textContent=r.error;return}
if(r.ready){T.className='msg ok';T.textContent='Enregistré ✓ L\'afficheur redémarre…';api('/api/reboot',{})}else{T.className='msg err';T.textContent='Enregistré. Il manque encore : '+r.missing+'.'}};
$('reset').onclick=async()=>{if(confirm('Effacer le WiFi, la clé et les arrêts ?')){await api('/api/reset',{});$('toast').textContent='Réglages effacés, redémarrage…'}};
load();setInterval(()=>status().catch(()=>{}),15000);
</script></body></html>)HTML";
