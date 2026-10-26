// =====================================================================
//  WebPage.h — página de controle (HTML + CSS + JavaScript)
//
//  Fica num arquivo .h separado de propósito: a Arduino IDE varre o .ino
//  procurando funções para criar protótipos e confundia as funções em
//  JavaScript daqui com funções C++ ("'asyncfunction' does not name a type").
// =====================================================================
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Cozmo-MK2</title>
<style>
/* A página rola normalmente; só o joystick e os botões de segurar bloqueiam o toque */
body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:10px 12px 24px;
     touch-action:manipulation;user-select:none;-webkit-user-select:none}
h2{margin:6px 0 2px;font-size:22px}
#st{font-size:14px;margin-bottom:12px}.ok{color:#4c4}.bad{color:#e55}
#joy{width:min(58vw,210px);height:min(58vw,210px);margin:0 auto;border-radius:50%;position:relative;touch-action:none;
  background:radial-gradient(circle,#1b2a3a 0,#14202c 60%,#0f1822 100%);border:2px solid #2a4a6a}
#joy::before,#joy::after{content:'';position:absolute;background:#24405c}
#joy::before{left:50%;top:14px;bottom:14px;width:2px;margin-left:-1px}
#joy::after{top:50%;left:14px;right:14px;height:2px;margin-top:-1px}
#knob{position:absolute;left:50%;top:50%;width:36%;height:36%;margin:-18% 0 0 -18%;border-radius:50%;
  background:#0a74da;box-shadow:0 4px 14px #0008;z-index:1;transition:transform .12s}
#joy.on #knob{transition:none;background:#1688f0}
details{max-width:340px;margin:22px auto 0;text-align:left;background:#191919;border:1px solid #333;border-radius:12px;padding:10px 12px}
summary{cursor:pointer;color:#aaa;font-size:15px}
#wst{font-size:13px;color:#bbb;margin:8px 0}#wst b{color:#fff}.warn{color:#f0a020}
.nets button{display:flex;width:100%;justify-content:space-between;background:#222;color:#eee;border:1px solid #333;border-radius:8px;padding:8px 10px;margin:4px 0;font-size:14px}
.f input{width:100%;box-sizing:border-box;margin:4px 0 8px;padding:9px;border-radius:8px;border:1px solid #444;background:#111;color:#eee;font-size:15px}
.f label{font-size:12px;color:#999}
.btn{background:#0a74da;color:#fff;border:0;border-radius:8px;padding:9px 14px;font-size:14px}
.btn.g{background:#333}
#wmsg{font-size:13px;margin-top:8px}
.cal p{font-size:13px;color:#bbb;margin:10px 0 6px}.cal b{color:#fff}
.trow{display:flex;align-items:center;gap:6px;margin:6px 0;font-size:13px}
.trow span.l{width:62px;color:#aaa}.trow input[type=range]{flex:1;width:auto}
.trow .v{width:52px;text-align:right;font-variant-numeric:tabular-nums}
.sb{background:#333;color:#eee;border:0;border-radius:6px;width:30px;height:30px;font-size:16px}
.hold{background:#2a3d52;color:#fff;border:0;border-radius:8px;padding:9px 10px;font-size:13px;margin:3px 2px}
.hold.on,.btn.on{background:#1f9d55}
.chk{font-size:13px;color:#ccc;display:block;margin:4px 0}.chk input{width:auto}
.b{background:#0a74da;border-radius:16px;display:flex;align-items:center;justify-content:center;font-size:32px}
.b.on{background:#005bb5;transform:scale(.95)}
#w{width:min(80vw,290px);height:48px;margin:12px auto;font-size:18px;touch-action:none}
.hold{touch-action:none}
h3{margin:14px 0 6px;font-size:15px;color:#aaa}
.chips{display:flex;flex-wrap:wrap;gap:8px;justify-content:center;max-width:340px;margin:auto}
.chips button{background:#222;color:#eee;border:1px solid #444;border-radius:20px;padding:9px 14px;font-size:15px}
.chips button.sel{background:#0a74da;border-color:#0a74da}
input{width:280px}
.autorow{display:flex;align-items:center;justify-content:center;gap:10px;font-size:14px;color:#aaa;margin:2px 0 4px}
.sw{width:40px;height:22px;border-radius:11px;background:#444;position:relative;transition:.2s;cursor:pointer}
.sw::after{content:'';position:absolute;top:3px;left:3px;width:16px;height:16px;border-radius:50%;background:#ccc;transition:.2s}
.sw.on{background:#1f9d55}.sw.on::after{left:21px;background:#fff}
#styles,#vopt{margin:0;gap:6px}#styles button,#vopt button{padding:4px 10px;font-size:12px}
</style></head><body>
<h2>Cozmo-MK2</h2><div id="st" class="bad">conectando…</div>
<div class="autorow">Andar sozinho <div id="auto" class="sw"></div>
 <span class="chips" id="styles" style="display:none"><button data-m="1">Inquieto</button><button data-m="2" class="sel">Curioso</button></span></div>
<div class="autorow">Modo vitrine <div id="vit" class="sw"></div>
 <span class="chips" id="vopt" style="display:none"><button id="vmov" class="sel">com movimento</button></span></div>
<h3>Controle</h3>
<div id="joy"><div id="knob"></div></div>
<div class="b" id="w" data-c="W">Wiggle</div>
<h3>Humor</h3><div class="chips" id="moods"></div>
<h3>Animações</h3><div class="chips" id="anims"></div>
<p>Velocidade: <span id="sv">100</span>%<br><input id="sp" type="range" min="20" max="100" value="100"></p>
<p style="font-size:12px;color:#888">Teclado: WASD / setas (dá para combinar), espaço = wiggle</p>
<details id="cal" class="cal"><summary>Calibrar rodas</summary>
 <p><b>1. Ponto de parada.</b> Ligue o teste: as rodas recebem o sinal de "parado". Se alguma girar devagar, mova o ajuste dela até parar de vez.</p>
 <button class="btn g" id="ctest">Testar parada</button>
 <div class="trow"><span class="l">Esquerda</span><button class="sb" data-s="L" data-d="-1">−</button><input type="range" id="tL" min="-100" max="100" value="0"><button class="sb" data-s="L" data-d="1">+</button><span class="v" id="vL">0 µs</span></div>
 <div class="trow"><span class="l">Direita</span><button class="sb" data-s="R" data-d="-1">−</button><input type="range" id="tR" min="-100" max="100" value="0"><button class="sb" data-s="R" data-d="1">+</button><span class="v" id="vR">0 µs</span></div>
 <p><b>2. Sentido.</b> Segure o botão: as duas rodas devem girar para a <b>frente</b>. A que girar para trás, marque "inverter".</p>
 <button class="hold" data-w="A">Segure: as duas p/ frente</button><button class="hold" data-w="L">Só esquerda</button><button class="hold" data-w="R">Só direita</button>
 <label class="chk"><input type="checkbox" id="iL"> inverter roda esquerda</label>
 <label class="chk"><input type="checkbox" id="iR"> inverter roda direita</label>
 <br><button class="btn" id="csave">Salvar calibração</button> <button class="btn g" id="creset">Padrão de fábrica</button>
 <div id="cmsg" style="font-size:13px;margin-top:8px"></div>
</details>
<details id="wifi"><summary>Wi-Fi</summary>
 <div id="wst">…</div>
 <button class="btn g" id="scan">Procurar redes</button>
 <div class="nets" id="nets"></div>
 <div class="f">
  <label>Rede</label><input id="ssid" autocomplete="off" autocapitalize="off">
  <label>Senha</label><input id="pass" type="password" autocomplete="off">
  <label style="font-size:12px"><input type="checkbox" id="show" style="width:auto"> mostrar senha</label><br><br>
  <button class="btn" id="save">Salvar e conectar</button>
  <button class="btn g" id="forget" style="display:none">Esquecer rede</button>
 </div>
 <div id="wmsg"></div>
</details>
<script>
let ws,cmd='S',timer=null;const st=document.getElementById('st');
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=()=>{st.textContent='conectado';st.className='ok'};
  ws.onclose=()=>{st.textContent='desconectado — tentando de novo…';st.className='bad';setTimeout(connect,1000)};
}
function send(m){if(ws&&ws.readyState===1)ws.send(m)}
// Enquanto algo estiver pressionado, o comando é reenviado a cada 120 ms (se parar de chegar, o robô para)
function hold(fn){clearInterval(timer);fn();timer=setInterval(fn,120)}
function stop(){clearInterval(timer);timer=null;send('S');
  document.querySelectorAll('.b.on').forEach(e=>e.classList.remove('on'))}
// Wiggle
const wb=document.getElementById('w');
wb.addEventListener('pointerdown',e=>{e.preventDefault();wb.setPointerCapture(e.pointerId);wb.classList.add('on');hold(()=>send('W'))});
['pointerup','pointercancel','lostpointercapture'].forEach(ev=>wb.addEventListener(ev,stop));
// Joystick: x/y de -100 a 100 (y+ = frente)
let jx=0,jy=0,lastJ=0;
const joy=document.getElementById('joy'),knob=document.getElementById('knob');
const sendJ=()=>{send('J'+jx+','+jy);lastJ=Date.now()};
function setJ(x,y){
  const m=Math.hypot(x,y); if(m>1){x/=m;y/=m}
  jx=Math.round(x*100); jy=Math.round(y*100);
  const R=(joy.clientWidth-knob.offsetWidth)/2;
  knob.style.transform='translate('+(x*R)+'px,'+(-y*R)+'px)';
  if(Date.now()-lastJ>60)sendJ();
}
function joyMove(e){const r=joy.getBoundingClientRect(),R=(r.width-knob.offsetWidth)/2;
  setJ((e.clientX-r.left-r.width/2)/R,-(e.clientY-r.top-r.height/2)/R)}
joy.addEventListener('pointerdown',e=>{e.preventDefault();joy.setPointerCapture(e.pointerId);joy.classList.add('on');joyMove(e);hold(sendJ)});
joy.addEventListener('pointermove',e=>{if(joy.classList.contains('on'))joyMove(e)});
['pointerup','pointercancel','lostpointercapture'].forEach(ev=>joy.addEventListener(ev,()=>{
  if(!joy.classList.contains('on'))return; joy.classList.remove('on'); jx=jy=0; knob.style.transform=''; stop()}));
// Teclado: setas/WASD viram o joystick (dá para combinar, ex.: frente + direita)
const down=new Set(), KEYS={ArrowUp:'u',w:'u',W:'u',ArrowDown:'d',s:'d',S:'d',ArrowLeft:'l',a:'l',A:'l',ArrowRight:'r',d:'r',D:'r'};
function keyJoy(){const x=(down.has('r')?1:0)-(down.has('l')?1:0), y=(down.has('u')?1:0)-(down.has('d')?1:0);
  if(x||y){setJ(x,y);if(!timer)hold(sendJ)} else {jx=jy=0;knob.style.transform='';stop()}}
addEventListener('keydown',e=>{if(e.target.tagName==='INPUT')return;
  if(e.key===' '){e.preventDefault();if(!e.repeat)hold(()=>send('W'));return}
  const k=KEYS[e.key]; if(k){e.preventDefault();if(!down.has(k)){down.add(k);keyJoy()}}});
addEventListener('keyup',e=>{if(e.key===' '){stop();return} const k=KEYS[e.key]; if(k){down.delete(k);keyJoy()}});
addEventListener('blur',()=>{down.clear();stop()});document.addEventListener('visibilitychange',()=>{if(document.hidden){down.clear();stop()}});
const sp=document.getElementById('sp');
sp.oninput=()=>{document.getElementById('sv').textContent=sp.value;send('V'+sp.value)};
// Lista de modos/emoções/animações vem do firmware: adicionou lá, aparece aqui
const MODE_HELP=['','Inquieto: dá umas mexidinhas de vez em quando','Curioso: explora sozinho — use no chão, longe de beiradas'];
// Botão "Andar sozinho": liga/desliga o modo autônomo; os chips escolhem o estilo
let autoOn=false, autoStyle=2;
const autoBtn=document.getElementById('auto');
function showAuto(){
  autoBtn.className='sw'+(autoOn?' on':'');
  document.getElementById('styles').style.display=autoOn?'inline-flex':'none';
  document.querySelectorAll('#styles button').forEach(b=>{b.className=(+b.dataset.m===autoStyle)?'sel':'';b.title=MODE_HELP[+b.dataset.m]});
}
autoBtn.onclick=()=>{autoOn=!autoOn;send('M'+(autoOn?autoStyle:0));if(autoOn&&vitOn){vitOn=false;showVit()}showAuto()};
// Modo vitrine: roteiro com todas as emoções e animações, em loop
let vitOn=false, vitMove=true;
function showVit(){
  document.getElementById('vit').className='sw'+(vitOn?' on':'');
  document.getElementById('vopt').style.display=vitOn?'inline-flex':'none';
  document.getElementById('vmov').className=vitMove?'sel':'';
  document.getElementById('vmov').textContent=vitMove?'com movimento':'só rosto';
}
document.getElementById('vit').onclick=()=>{vitOn=!vitOn;send('Y'+(vitOn?(vitMove?1:2):0));if(vitOn&&autoOn){autoOn=false;showAuto()}showVit()};
document.getElementById('vmov').onclick=()=>{vitMove=!vitMove;if(vitOn)send('Y'+(vitMove?1:2));showVit()};
document.querySelectorAll('#styles button').forEach(b=>b.onclick=()=>{
  autoStyle=+b.dataset.m; if(autoOn)send('M'+autoStyle); showAuto()});
fetch('/faces').then(r=>r.json()).then(f=>{
  const mk=(box,list,prefix,sel)=>list.forEach((n,i)=>{
    const b=document.createElement('button');b.textContent=n;
    if(i===sel)b.className='sel';
    b.onclick=()=>{send(prefix+i);if(sel>=0){box.querySelectorAll('button').forEach(x=>x.className='');b.className='sel'}
};
    box.appendChild(b)});
  if(f.mode>0){autoOn=true;autoStyle=f.mode} showAuto();
  if(f.show>0){vitOn=true;vitMove=f.show===1} showVit();
  mk(document.getElementById('moods'),f.moods,'E',f.mood);
  mk(document.getElementById('anims'),f.anims,'A',-1);
});
// ---------------- Wi-Fi ----------------
const $=id=>document.getElementById(id);
const esc=t=>t.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
async function wifiStatus(){
  try{const s=await (await fetch('/wifi/status')).json();
    let h=s.mode==='ap'
      ?'Usando a rede própria do robô (<b>'+esc(s.ssid)+'</b>). Escolha a rede da sua casa abaixo — assim seu celular continua com internet.'
      :'Conectado em <b>'+esc(s.ssid)+'</b> · '+s.ip+'<br>Também dá para abrir em <b>http://'+s.host+'</b>';
    if(s.lost)h+='<br><span class="warn">A rede "'+esc(s.lost)+'" caiu. Quando ela voltar, o robô entra nela sozinho.</span>';
    else if(s.fail)h+='<br><span class="warn">Não consegui entrar em "'+esc(s.fail)+'" ao ligar (senha errada ou rede fora do ar). Se a rede estiver ok, confira a senha.</span>';
    $('wst').innerHTML=h; $('forget').style.display=(s.mode==='sta'||s.fail)?'':'none';
    if(s.mode==='ap')$('wifi').open=true;
  }catch(e){}
}
const bars=r=>r>-55?'▂▄▆█':r>-67?'▂▄▆':r>-78?'▂▄':'▂';
$('scan').onclick=async()=>{
  $('scan').disabled=true;$('nets').innerHTML='<small>procurando… (a conexão pode piscar por 2 s)</small>';
  try{await fetch('/wifi/scan?start=1')}catch(e){}
  for(let i=0;i<30;i++){await new Promise(r=>setTimeout(r,700));
    try{const j=await (await fetch('/wifi/scan')).json(); if(j.scanning)continue;
      $('nets').innerHTML='';
      if(!j.nets.length)$('nets').innerHTML='<small>nenhuma rede encontrada</small>';
      j.nets.forEach(n=>{const b=document.createElement('button');
        b.innerHTML='<span>'+esc(n.ssid)+(n.lock?' 🔒':'')+'</span><span>'+bars(n.rssi)+'</span>';
        b.onclick=()=>{$('ssid').value=n.ssid;$('pass').value='';$('pass').focus()};$('nets').appendChild(b)});
      break;}catch(e){}
  }
  $('scan').disabled=false;
};
$('show').onchange=()=>$('pass').type=$('show').checked?'text':'password';
$('save').onclick=async()=>{
  const ssid=$('ssid').value.trim(),pass=$('pass').value;
  if(!ssid){$('wmsg').textContent='Escolha ou digite o nome da rede.';return}
  if(pass&&pass.length<8){$('wmsg').textContent='A senha do Wi-Fi tem pelo menos 8 caracteres.';return}
  const r=await fetch('/wifi/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)});
  $('wmsg').innerHTML=r.ok
    ?'<span class="ok">Salvo! O robô vai reiniciar e entrar em "'+esc(ssid)+'".</span><br>Conecte seu celular nessa rede e abra <b>http://cozmo.local</b> ou o endereço que aparecer no display. Se não conectar, ele volta para a rede própria em 30 s. Não abriu? Use o endereço (números) que aparece no display, porque o Android nem sempre entende o "cozmo.local".'
    :'<span class="bad">'+esc(await r.text())+'</span>';
};
$('forget').onclick=async()=>{
  if(!confirm('Esquecer a rede de casa? O robô reinicia usando a rede própria.'))return;
  await fetch('/wifi/forget',{method:'POST'});
  $('wmsg').innerHTML='Rede esquecida. Reiniciando… conecte-se na rede <b>Cozmo-MK2</b>.';
};
// ---------------- Calibração ----------------
const DEF={ldir:1,rdir:-1};      // sentidos de fábrica (iguais ao firmware)
let cal={ltrim:0,rtrim:0,ldir:1,rdir:-1}, calTest=false;
function showCal(){
  for(const S of ['L','R']){const t=S==='L'?cal.ltrim:cal.rtrim;$('t'+S).value=t;$('v'+S).textContent=(t>0?'+':'')+t+' µs'}
  $('iL').checked=cal.ldir!==DEF.ldir; $('iR').checked=cal.rdir!==DEF.rdir;
}
async function loadCal(){try{cal=await (await fetch('/calib')).json();showCal()}catch(e){}}
function setTrim(S,v){v=Math.max(-100,Math.min(100,v)); if(S==='L')cal.ltrim=v; else cal.rtrim=v; send('KT'+S+v); showCal(); $('cmsg').textContent=''}
['L','R'].forEach(S=>$('t'+S).oninput=e=>setTrim(S,+e.target.value));
document.querySelectorAll('.sb').forEach(b=>b.onclick=()=>setTrim(b.dataset.s,(b.dataset.s==='L'?cal.ltrim:cal.rtrim)+(+b.dataset.d)*2));
$('ctest').onclick=()=>{calTest=!calTest;$('ctest').classList.toggle('on',calTest);$('ctest').textContent=calTest?'Parar teste':'Testar parada';
  if(calTest)hold(()=>send('KWA0')); else stop()};
document.querySelectorAll('.hold').forEach(b=>{
  b.addEventListener('pointerdown',e=>{e.preventDefault();b.setPointerCapture(e.pointerId);b.classList.add('on');
    if(calTest){calTest=false;$('ctest').classList.remove('on');$('ctest').textContent='Testar parada'}
    hold(()=>send('KW'+b.dataset.w+'40'))});
  ['pointerup','pointercancel','lostpointercapture'].forEach(ev=>b.addEventListener(ev,()=>{if(b.classList.contains('on')){b.classList.remove('on');stop()}}));
});
$('iL').onchange=()=>{cal.ldir=$('iL').checked?-DEF.ldir:DEF.ldir;send('KDL'+cal.ldir)};
$('iR').onchange=()=>{cal.rdir=$('iR').checked?-DEF.rdir:DEF.rdir;send('KDR'+cal.rdir)};
$('csave').onclick=()=>{send('KS');$('cmsg').innerHTML='<span class="ok">Calibração salva — vale mesmo depois de desligar.</span>'};
$('creset').onclick=()=>{if(!confirm('Voltar a calibração para o padrão de fábrica?'))return;send('KZ');setTimeout(loadCal,400);$('cmsg').textContent='Padrão de fábrica restaurado.'};
$('cal').addEventListener('toggle',()=>{if($('cal').open)loadCal(); else if(calTest){calTest=false;$('ctest').classList.remove('on');$('ctest').textContent='Testar parada';stop()}});

wifiStatus();
connect();
</script></body></html>
)HTML";
