// =====================================================================
//  Cozmo-MK2 — versão revisada com sistema de emoções
//  - Olhos procedurais (Eyes.h): emoções = parâmetros, transição suave,
//    piscada automática, olhar reage à direção
//  - Animações em bitmap (anim_*.h) geradas por tools/gif2header.py
//  - Controle por WebSocket com "dead-man switch"
//  - Tudo não-bloqueante (millis); fallback para Access Point + mDNS
// =====================================================================
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "Eyes.h"
#include "Autonomy.h"
#include "Showcase.h"
#include "WebPage.h"
#include "BitmapAnim.h"
#include "anim_shush.h"
#include "anim_apaixonados.h"
#include "anim_raiva.h"
#include "anim_sono.h"
#include "anim_surpresa.h"
#include "anim_medo.h"
#include "anim_bocejo.h"
#include "anim_cansado.h"
#include "anim_confuso.h"
#include "anim_micro_olhar_lateral.h"
#include "anim_micro_piscada_dupla.h"
#include "anim_micro_piscar_um.h"
#include "anim_micro_sorriso.h"
#include "anim_micro_suspiro.h"
#include "anim_micro_desconfianca.h"
#include "anim_micro_sobressalto.h"
// #include "anim_nova.h"          // <- novas animações: inclua aqui...

// Animações com botão na página de controle (registre aqui as novas)
const BitmapAnim* const ANIMS[] = {
  &SHUSH_ANIM,
  &APAIXONADOS_ANIM,
  &RAIVA_ANIM,
  &SONO_ANIM,
  &SURPRESA_ANIM,
  &MEDO_ANIM,
  &BOCEJO_ANIM,
  &CANSADO_ANIM,
  &CONFUSO_ANIM,
  // &NOVA_ANIM,
};
const uint8_t ANIM_COUNT = sizeof(ANIMS) / sizeof(ANIMS[0]);

// Microexpressões: SEM botão — entram sozinhas no meio dos olhos para dar vida
const BitmapAnim* const MICROS[] = {
  &MICRO_OLHAR_LATERAL_ANIM,
  &MICRO_PISCADA_DUPLA_ANIM,
  &MICRO_PISCAR_UM_ANIM,
  &MICRO_SORRISO_ANIM,
  &MICRO_SUSPIRO_ANIM,
  &MICRO_DESCONFIANCA_ANIM,
  &MICRO_SOBRESSALTO_ANIM,
};
const uint8_t MICRO_COUNT = sizeof(MICROS) / sizeof(MICROS[0]);
const BitmapAnim* animAt(int i) { return i < ANIM_COUNT ? ANIMS[i] : MICROS[i - ANIM_COUNT]; }
uint32_t animMs(const char* name);   // duração de uma animação (definida mais abaixo)

// Qual animação toca em cada reação automática (pelo nome; "" = nenhuma)
const char* const REACTION_ANIM[] = {
  "surpresa",     // REACT_SURPRISE: recua assustado / é acordado
  "apaixonados",  // REACT_LOVE:     alguém abriu a página / depois da dancinha
  "raiva",        // REACT_ANGRY:    desligaram o "Andar sozinho"
  "sono",         // REACT_SLEEP:    pegou no sono
  "medo",         // REACT_FEAR:     correndo rápido demais / susto grande no modo Curioso
  "bocejo",       // REACT_YAWN:     alguns segundos antes de dormir
  "cansado",      // REACT_TIRED:    depois de correr muito
  "confuso",      // REACT_CONFUSED: Wi-Fi caiu
};
const uint32_t REACT_COOLDOWN_MS = 4000;   // intervalo mínimo entre reações automáticas

// ---------------- Configuração ----------------
// Rede de casa: o jeito fácil é configurar pela página (seção "Wi-Fi"), que fica salvo na memória.
// Estes valores só são usados se nada tiver sido salvo pela página.
const char* WIFI_SSID = "WIFI NAME";
const char* WIFI_PASS = "PASSWORD";

const char* AP_SSID  = "Cozmo-MK2";
const char* AP_PASS  = "cozmo1234";         // mínimo 8 caracteres
const char* HOSTNAME = "cozmo";             // http://cozmo.local
const uint32_t WIFI_TIMEOUT_MS = 30000;      // tempo tentando a rede de casa antes de criar a própria
const uint8_t  AP_CHANNEL      = 6;         // canal fixo da rede própria
const uint32_t HOME_LOST_MS    = 30000;     // rede de casa caiu com ele ligado: depois disso cria a própria
const uint32_t HOME_RETRY_MS   = 60000;     // na rede própria, procura a de casa a cada 1 min (se ninguém estiver conectado)
const uint8_t  HOME_MAX_RETRY  = 3;         // tentativas automáticas antes de desistir (evita reiniciar sem parar com senha errada)
// A ESP32-C3 SuperMini tem antena fraca/mal casada: na potência máxima muitas placas ficam instáveis.
// 8,5 dBm costuma resolver. Se o alcance ficar curto, tente WIFI_POWER_11dBm ou WIFI_POWER_15dBm.
const wifi_power_t WIFI_TX_POWER = WIFI_POWER_8_5dBm;

// Pinos (ESP32-C3 SuperMini)
const int LEFT_PIN = 6, RIGHT_PIN = 7, SDA_PIN = 8, SCL_PIN = 9;

// Servos de rotação contínua, controlados por largura de pulso:
//   1500 µs = parado | 1000 µs = velocidade máxima num sentido | 2000 µs = no outro
const int SERVO_STOP_US  = 1500;
const int SERVO_RANGE_US = 500;
// Calibração: o jeito fácil é pela página (seção "Calibrar rodas"), que salva na memória.
// Estes são só os valores de fábrica, usados até você salvar uma calibração.
const int DEF_LEFT_TRIM = 0, DEF_RIGHT_TRIM = 0;   // µs: ajuste fino do ponto de parada
const int DEF_LEFT_DIR = +1, DEF_RIGHT_DIR = -1;   // sentido de cada roda
int leftTrim = DEF_LEFT_TRIM, rightTrim = DEF_RIGHT_TRIM;
int leftDir  = DEF_LEFT_DIR,  rightDir  = DEF_RIGHT_DIR;
// Parado = sem pulso: o servo desliga e não fica "arrastando" nem consumindo bateria
const uint32_t SERVO_RELEASE_MS = 300;

// Joystick
const float JOY_DEADZONE = 0.10f;   // ignora toques pequenos no centro
const float JOY_EXPO     = 1.5f;    // >1 = mais precisão em baixa velocidade
const float JOY_TURN     = 0.8f;    // quanto o eixo X faz girar (1 = giro total)
const uint32_t CMD_TIMEOUT_MS = 400;        // sem comando -> para
const uint32_t FRAME_MS       = 33;         // ~30 fps (limite real do I2C a 400 kHz)
const uint32_t WIGGLE_MS      = 100;
const uint32_t SLEEP_AFTER_MS = 30000;      // parado por 30 s -> fica com sono
const uint32_t YAWN_BEFORE_MS = 3000;       // boceja 3 s antes de dormir

// Cansaço: cada segundo andando em velocidade máxima soma 1 ponto; parado, recupera
const float    TIRED_AT        = 25.0f;     // ~25 s correndo a toda -> cansado
const float    TIRED_RECOVER   = 0.5f;      // pontos recuperados por segundo parado
const uint32_t TIRED_PANT_MS   = 7000;      // enquanto cansado e parado, ofega de novo a cada 7 s

// Medo: correr em alta velocidade sem parar
const int      FEAR_SPEED_PCT  = 90;        // velocidade a partir da qual ele se assusta
const uint32_t FEAR_AFTER_MS   = 2500;      // ...depois de tantos ms reto nessa velocidade

// Microexpressões
const uint32_t MICRO_MIN_MS = 6000, MICRO_MAX_MS = 15000;   // intervalo entre uma e outra
const uint32_t STARTLE_IDLE_MS = 10000;     // parado tanto tempo e alguém mexe -> sobressalto

// ---------------- Hardware ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo leftServo, rightServo;
Eyes eyes;
Autonomy autonomy;
Showcase show;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- Estado (compartilhado com a task do servidor) ----------------
enum Cmd : char { STOP = 'S', FWD = 'F', BACK = 'B', LEFT = 'L', RIGHT = 'R', WIGGLE = 'W', JOY = 'J', CAL = 'K' };
volatile char calWheel = 'A';                // teste de calibração: 'L', 'R' ou 'A' (ambas)
volatile int  calSpeed = 0;                  // -100..100; 0 = pulso de parada (para ver se a roda arrasta)
volatile bool calSaveReq = false, calResetReq = false;
volatile int joyX = 0, joyY = 0;            // joystick da página: -100..100 (y+ = frente, x+ = direita)
volatile char     currentCmd = STOP;
volatile uint32_t lastCmdMs  = 0;
volatile uint32_t lastMoveMs = 0;
volatile int      speedPct   = 100;
volatile bool     clientSeen = false;
volatile uint8_t  baseMood   = NEUTRAL;     // emoção escolhida na página
volatile int      animReq    = -1;          // animação pedida pela página
volatile uint32_t lastManualMs = 0;         // último comando de movimento manual
volatile int      modeReq    = -1;          // modo autônomo pedido pela página
uint32_t          bannerUntil = 0;
const char*       bannerText  = "";
volatile int      showReq    = -1;          // vitrine: 0 = desliga, 1 = com movimento, 2 = só rosto
volatile int8_t   reactReq   = REACT_NONE;  // reação pedida (servidor ou troca de modo)
const uint32_t    AUTO_RESUME_MS = 4000;    // depois de usar o controle, volta ao modo autônomo

bool apMode = false;
String ipText;

// ---------------- Motores ----------------
bool servosOn = false;
uint32_t stoppedSince = 0;

// Mantém os pinos em nível baixo: sem pulso, o servo fica parado
void servosRelease() {
  if (servosOn) {
    leftServo.detach();
    rightServo.detach();
    servosOn = false;
  }
  pinMode(LEFT_PIN, OUTPUT);  digitalWrite(LEFT_PIN, LOW);
  pinMode(RIGHT_PIN, OUTPUT); digitalWrite(RIGHT_PIN, LOW);
}

void servosAttach() {
  if (servosOn) return;
  leftServo.setPeriodHertz(50);
  rightServo.setPeriodHertz(50);
  leftServo.attach(LEFT_PIN, 1000, 2000);
  rightServo.attach(RIGHT_PIN, 1000, 2000);
  servosOn = true;
}

// v de -1.0 (ré total) a +1.0 (frente total) para cada roda
float effort = 0;                                  // cansaço acumulado (ver TIRED_AT)

void setWheels(float l, float r) {
  const uint32_t now = millis();
  static uint32_t lastEffortMs = 0;
  const float dt = lastEffortMs ? (now - lastEffortMs) / 1000.0f : 0;
  lastEffortMs = now;
  const float v = max(fabsf(l), fabsf(r)) * speedPct / 100.0f;
  effort = (v > 0) ? effort + dt * v : max(0.0f, effort - dt * TIRED_RECOVER);
  if (l == 0 && r == 0) {                          // parar = cortar o pulso
    if (servosOn) {
      if (stoppedSince == 0) {
        stoppedSince = now;
        leftServo.writeMicroseconds(SERVO_STOP_US + leftTrim);
        rightServo.writeMicroseconds(SERVO_STOP_US + rightTrim);
      } else if (now - stoppedSince > SERVO_RELEASE_MS) {
        servosRelease();
      }
    }
    return;
  }
  stoppedSince = 0;
  servosAttach();
  const float k = SERVO_RANGE_US * speedPct / 100.0f;
  leftServo.writeMicroseconds (SERVO_STOP_US + leftTrim  + (int)(leftDir  * l * k));
  rightServo.writeMicroseconds(SERVO_STOP_US + rightTrim + (int)(rightDir * r * k));
}

char activeCmd(uint32_t now) {
  return (now - lastCmdMs > CMD_TIMEOUT_MS) ? STOP : currentCmd;   // dead-man switch
}

void updateDrive(uint32_t now) {
  int m = modeReq;
  if (m >= 0) {
    modeReq = -1;
    if (m == AUTO_OFF && autonomy.getMode() != AUTO_OFF) reactReq = REACT_ANGRY;  // foi parado
    autonomy.setMode((AutoMode)m);
    if (m != AUTO_OFF) show.stop();                   // andar sozinho e vitrine não rodam juntos
    bannerText = AUTO_MODE_NAMES[m];
    bannerUntil = now + 1500;                         // mostra o nome do modo na faixa amarela
  }
  int sr = showReq;
  if (sr >= 0) {
    showReq = -1;
    if (sr == 0) show.stop();
    else { autonomy.setMode(AUTO_OFF); show.start(now, sr == 1); }
    bannerText = sr ? "Vitrine" : "Manual";
    bannerUntil = now + 1500;
  }

  const char cmd = activeCmd(now);
  // Controle manual sempre tem prioridade; o autônomo volta alguns segundos depois
  const bool manualRecent = lastManualMs != 0 && now - lastManualMs < AUTO_RESUME_MS;
  if (cmd == STOP && !manualRecent && show.isActive()) {
    show.update(now, animMs);
    lastMoveMs = now;                                 // na vitrine ele não dorme de verdade
    setWheels(show.wheelL, show.wheelR);
    return;
  }
  show.hold(now);                                     // controle manual pausa a vitrine
  if (cmd == STOP && !manualRecent) {
    autonomy.update(now);
    if (autonomy.getMode() != AUTO_OFF) lastMoveMs = now;   // no modo autônomo ele não dorme
    setWheels(autonomy.wheelL, autonomy.wheelR);
    return;
  }
  autonomy.reset(now);

  switch (cmd) {
    case FWD:    setWheels( 1,  1); break;
    case BACK:   setWheels(-1, -1); break;
    case LEFT:   setWheels(-1,  1); break;
    case RIGHT:  setWheels( 1, -1); break;
    case WIGGLE: ((now / WIGGLE_MS) & 1) ? setWheels(-1, 1) : setWheels(1, -1); break;
    case CAL: {
      // Teste de calibração: manda o pulso direto (sem cortar o sinal quando parado),
      // assim dá para ver se a roda "arrasta" no ponto de parada
      stoppedSince = 0;
      servosAttach();
      const int dl = (calWheel == 'R') ? 0 : calSpeed * 3;   // até ±300 µs: velocidade de teste moderada
      const int dr = (calWheel == 'L') ? 0 : calSpeed * 3;
      leftServo.writeMicroseconds (SERVO_STOP_US + leftTrim  + leftDir  * dl);
      rightServo.writeMicroseconds(SERVO_STOP_US + rightTrim + rightDir * dr);
      break;
    }
    case JOY: {
      // Mistura diferencial: frente/trás + giro. Centro com zona morta, curva suave nas pontas.
      float x = joyX / 100.0f, y = joyY / 100.0f;
      if (fabsf(x) < JOY_DEADZONE) x = 0;
      if (fabsf(y) < JOY_DEADZONE) y = 0;
      x = copysignf(powf(fabsf(x), JOY_EXPO), x) * JOY_TURN;
      y = copysignf(powf(fabsf(y), JOY_EXPO), y);
      float l = y + x, r = y - x;
      const float m = max(1.0f, max(fabsf(l), fabsf(r)));
      setWheels(l / m, r / m);
      break;
    }
    default:     setWheels(0, 0);
  }
}

// ---------------- Reações automáticas ----------------
int findAnim(const char* name) {
  for (int i = 0; i < ANIM_COUNT + MICRO_COUNT; i++) if (strcmp(animAt(i)->name, name) == 0) return i;
  return -1;
}

uint32_t lastAnimEnd = 0;

// Duração total de uma animação em bitmap (usada pela vitrine para esperar ela acabar)
uint32_t animMs(const char* name) {
  int i = findAnim(name);
  if (i < 0) return 0;
  const BitmapAnim* a = animAt(i);
  uint32_t t = 0;
  for (uint16_t k = 0; k < a->len; k++) t += pgm_read_word(&a->ms[k]);
  return t;
}                           // quando terminou a última animação em bitmap

// force = ignora o intervalo mínimo (usado no sono, que vem logo depois do bocejo)
void playReaction(int8_t r, uint32_t now, bool force = false) {
  static uint32_t lastReact = 0;
  if (r < 0 || r >= (int8_t)(sizeof(REACTION_ANIM) / sizeof(REACTION_ANIM[0]))) return;
  if (!force && lastReact != 0 && now - lastReact < REACT_COOLDOWN_MS) return;   // não fica repetindo
  int a = findAnim(REACTION_ANIM[r]);
  if (a < 0 || animReq >= 0) return;
  animReq = a;
  lastReact = now;
}

// Sorteia uma microexpressão que combine com o humor atual (-1 = nenhuma)
int pickMicro(Emotion mood, uint32_t idleMs) {
  struct W { const char* name; uint8_t w; };
  static const W neutral[] = { {"micro_olhar_lateral",3}, {"micro_piscada_dupla",3}, {"micro_piscar_um",1},
                               {"micro_sorriso",1}, {"micro_suspiro",1} };
  static const W happy[]   = { {"micro_sorriso",3}, {"micro_piscar_um",2}, {"micro_piscada_dupla",1} };
  static const W sad[]     = { {"micro_suspiro",3}, {"micro_olhar_lateral",1} };
  static const W wary[]    = { {"micro_desconfianca",3}, {"micro_olhar_lateral",1} };
  static const W surpr[]   = { {"micro_piscada_dupla",1} };
  const W* list; uint8_t n;
  switch (mood) {
    case NEUTRAL:    list = neutral; n = 5; break;
    case HAPPY:      list = happy;   n = 3; break;
    case SAD:        list = sad;     n = 2; break;
    case ANGRY:
    case SUSPICIOUS: list = wary;    n = 2; break;
    case SURPRISED:  list = surpr;   n = 1; break;
    default:         return -1;                       // dormindo, tonto: nada de micro
  }
  int total = 0;
  for (uint8_t i = 0; i < n; i++) {
    if (strcmp(list[i].name, "micro_suspiro") == 0 && mood == NEUTRAL && idleMs < 10000) continue;  // suspira só depois de um tempo parado
    total += list[i].w;
  }
  if (total == 0) return -1;
  int r = random(total);
  for (uint8_t i = 0; i < n; i++) {
    if (strcmp(list[i].name, "micro_suspiro") == 0 && mood == NEUTRAL && idleMs < 10000) continue;
    if ((r -= list[i].w) < 0) return findAnim(list[i].name);
  }
  return -1;
}

// ---------------- Rosto ----------------
// Reações automáticas por cima da emoção escolhida
void updateFaceState(uint32_t now) {
  char cmd = activeCmd(now);
  Emotion mood = (Emotion)baseMood;

  // Vitrine: o roteiro manda no rosto (sem micros nem reações automáticas)
  if (show.isActive()) {
    reactReq = REACT_NONE;
    autonomy.wantReact = REACT_NONE;
    eyes.setEmotion(cmd == WIGGLE ? HAPPY : (Emotion)show.mood);
    if (cmd == STOP && show.hasLook) eyes.lookAt(show.lookX, show.lookY);
    if (cmd == JOY) eyes.lookAt(joyX / 100.0f, -joyY / 160.0f);
    if (show.wantAnim) {
      int a = findAnim(show.wantAnim);
      show.wantAnim = nullptr;
      if (a >= 0) animReq = a;
    }
    return;
  }

  // Wiggle por mais de 1 s deixa ele tonto por 3 s depois que para
  static uint32_t wiggleStart = 0, dizzyUntil = 0;
  if (cmd == WIGGLE) {
    if (wiggleStart == 0) wiggleStart = now;
    if (now - wiggleStart > 1000) dizzyUntil = now + 3000;
  } else {
    wiggleStart = 0;
  }

  if (cmd == WIGGLE)                                         mood = HAPPY;
  else if ((int32_t)(dizzyUntil - now) > 0)                  mood = DIZZY;
  else if (cmd == STOP && autonomy.mood >= 0)                mood = (Emotion)autonomy.mood;
  else if (cmd == STOP && now - lastMoveMs > SLEEP_AFTER_MS) mood = SLEEPY;

  // Cansado: depois de correr muito fica de pálpebra pesada e ofega quando para
  static bool tired = false;
  static uint32_t lastPant = 0;
  if (!tired && effort >= TIRED_AT) { tired = true; lastPant = 0; }
  if (tired && effort < TIRED_AT * 0.4f) tired = false;
  if (tired && mood == (Emotion)baseMood && cmd == STOP) {
    mood = SLEEPY;
    if (lastPant == 0 || now - lastPant > TIRED_PANT_MS) { playReaction(REACT_TIRED, now, lastPant == 0); lastPant = now; }
  }

  // Wi-Fi caiu (só quando conectado à rede de casa): fica confuso até voltar
  static bool wifiWasOk = true;
  static uint32_t lastConfused = 0;
  const bool wifiOk = apMode || WiFi.status() == WL_CONNECTED;
  if (!wifiOk) {
    mood = SUSPICIOUS;
    if (now - lastConfused > 4000) { playReaction(REACT_CONFUSED, now, true); lastConfused = now; }
  } else if (!wifiWasOk) {
    int a = findAnim("micro_sorriso");                // voltou: sorrisinho de alívio
    if (a >= 0 && animReq < 0) animReq = a;
  }
  wifiWasOk = wifiOk;

  // Medo: andando reto em alta velocidade por muito tempo
  static uint32_t fastSince = 0, lastFear = 0;
  const bool straightFast = ((cmd == FWD || cmd == BACK) && speedPct >= FEAR_SPEED_PCT) ||
                            (cmd == JOY && abs(joyY) * speedPct / 100 >= FEAR_SPEED_PCT && abs(joyX) < 30);
  if (straightFast) {
    if (fastSince == 0) fastSince = now;
    if (now - fastSince > FEAR_AFTER_MS && (lastFear == 0 || now - lastFear > 30000)) {  // no máximo 1x a cada 30 s
      playReaction(REACT_FEAR, now);
      lastFear = now;
    }
  } else {
    fastSince = 0;
  }

  // Bocejo alguns segundos antes de dormir, depois o "sono"
  static bool yawned = false;
  const uint32_t idle = now - lastMoveMs;
  if (cmd == STOP && mood != SLEEPY && !yawned && idle > SLEEP_AFTER_MS - YAWN_BEFORE_MS) {
    playReaction(REACT_YAWN, now, true);
    yawned = true;
  }
  if (idle < 1000) yawned = false;
  // "Dormindo" de verdade = sono por ficar parado (não conta o olhar de cansado)
  static bool wasAsleep = false;
  const bool asleep = mood == SLEEPY && !tired;
  if (asleep && !wasAsleep)                  playReaction(REACT_SLEEP, now, true);  // pegou no sono
  if (wasAsleep && !asleep && cmd != STOP)   playReaction(REACT_SURPRISE, now);     // foi acordado
  wasAsleep = asleep;
  eyes.setEmotion(mood);

  // Sobressalto: estava parado há um tempo (acordado) e alguém mexe no controle
  static char prevCmd = STOP;
  static uint32_t stillSince = 0;
  if (cmd == STOP && prevCmd != STOP) stillSince = now;
  if (cmd != STOP && prevCmd == STOP && stillSince != 0 && now - stillSince > STARTLE_IDLE_MS && mood != SLEEPY) {
    int a = findAnim("micro_sobressalto");
    if (a >= 0 && animReq < 0) animReq = a;
  }
  prevCmd = cmd;

  // Microexpressões espontâneas: só com o rosto "livre" (parado, sem outra animação)
  static uint32_t nextMicro = 0;
  if (nextMicro == 0) nextMicro = now + random(MICRO_MIN_MS, MICRO_MAX_MS);
  if (now - lastAnimEnd < 2000) nextMicro = max(nextMicro, now + 2000);   // respira depois de uma animação
  if ((int32_t)(now - nextMicro) >= 0) {
    nextMicro = now + random(MICRO_MIN_MS, MICRO_MAX_MS);
    if (cmd == STOP && !autonomy.moving() && animReq < 0) {
      int a = pickMicro(mood, idle);
      if (a >= 0) animReq = a;
    }
  }

  if (autonomy.wantReact != REACT_NONE) { playReaction(autonomy.wantReact, now); autonomy.wantReact = REACT_NONE; }
  int8_t rq = reactReq;
  if (rq != REACT_NONE) { reactReq = REACT_NONE; playReaction(rq, now, rq == REACT_ANGRY); }  // desligar sempre mostra a raiva

  if (cmd == STOP && autonomy.hasLook) eyes.lookAt(autonomy.lookX, autonomy.lookY);
  if (autonomy.wantBlink) { autonomy.wantBlink = false; eyes.blink(); }

  switch (cmd) {                                   // olha para onde vai
    case LEFT:  eyes.lookAt(-1, 0);    break;
    case RIGHT: eyes.lookAt( 1, 0);    break;
    case FWD:   eyes.lookAt( 0, -0.4); break;
    case BACK:  eyes.lookAt( 0, 0.6);  break;
    case JOY:   eyes.lookAt(joyX / 100.0f, -joyY / 160.0f); break;   // olha para onde o joystick aponta
    default: break;                                // parado: Eyes olha em volta sozinho
  }
}

// Toca uma animação bitmap; devolve false quando termina
bool updateBitmapAnim(const BitmapAnim* a, uint32_t now, bool restart) {
  static uint16_t step = 0;
  static uint32_t stepStart = 0;
  if (restart) { step = 0; stepStart = now; }
  else if (now - stepStart >= pgm_read_word(&a->ms[step])) {
    stepStart = now;
    if (++step >= a->len) return false;
  }
  display.clearDisplay();
  display.drawBitmap(0, 0, a->frames[pgm_read_word(&a->seq[step])], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.display();
  return true;
}

void updateDisplay(uint32_t now) {
  static uint32_t lastFrame = 0;
  static int playing = -1;
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  int req = animReq;
  if (req >= 0) {                                   // pedido novo de animação
    animReq = -1;
    playing = req;
    updateBitmapAnim(animAt(playing), now, true);
    return;
  }
  if (playing >= 0) {
    if (updateBitmapAnim(animAt(playing), now, false)) return;
    if (playing < ANIM_COUNT) eyes.blink();         // micro já termina com o olho aberto: sem piscada extra
    playing = -1;                                   // terminou: volta aos olhos
    lastAnimEnd = now;
  }

  updateFaceState(now);
  eyes.update(now);
  display.clearDisplay();
  eyes.draw(SSD1306_WHITE, SSD1306_BLACK);
  if ((int32_t)(bannerUntil - now) > 0) {             // nome do modo na faixa amarela
    const char* name = bannerText;
    display.fillRect(0, 0, SCREEN_WIDTH, TOP_ZONE, SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor((SCREEN_WIDTH - 6 * strlen(name)) / 2, 4);
    display.print(name);
  }
  display.display();
}

void showMessage(const char* l1, const char* l2 = "", const char* l3 = "", const char* l4 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(l1);
  display.println(l2);
  display.println(l3);
  display.println(l4);
  display.display();
}

// ---------------- Página web ----------------
// A página de controle está em WebPage.h

String facesJson() {
  String j = "{\"mood\":" + String((int)baseMood) + ",\"moods\":[";
  for (int i = 0; i < EMOTION_COUNT; i++) { if (i) j += ','; j += '"'; j += EMOTION_NAMES[i]; j += '"'; }
  j += "],\"anims\":[";
  for (int i = 0; i < ANIM_COUNT; i++)    { if (i) j += ','; j += '"'; j += ANIMS[i]->name; j += '"'; }
  j += "],\"mode\":" + String((int)autonomy.getMode()) + ",\"modes\":[";
  for (int i = 0; i < AUTO_MODE_COUNT; i++) { if (i) j += ','; j += '"'; j += AUTO_MODE_NAMES[i]; j += '"'; }
  j += "],\"show\":" + String(show.isActive() ? (show.motion ? 1 : 2) : 0);
  return j + "}";
}

// ---------------- WebSocket ----------------
int parseNum(const uint8_t* data, size_t len) {
  char buf[6] = {0};
  memcpy(buf, data + 1, min(len - 1, sizeof(buf) - 1));   // payload não vem com '\0'
  return atoi(buf);
}

void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
               void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    clientSeen = true;
    reactReq = REACT_LOVE;                            // alguém abriu a página: "oi!"
    lastMoveMs = millis();
  } else if (type == WS_EVT_DISCONNECT) {
    currentCmd = STOP;
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (!info->final || info->index != 0 || info->opcode != WS_TEXT || len == 0) return;
    const char c = data[0];
    if (c == 'V' && len > 1) {
      speedPct = constrain(parseNum(data, len), 20, 100);
    } else if (c == 'E' && len > 1) {
      int e = parseNum(data, len);
      if (e >= 0 && e < EMOTION_COUNT) { baseMood = e; lastMoveMs = millis(); }
    } else if (c == 'A' && len > 1) {
      int a = parseNum(data, len);
      if (a >= 0 && a < ANIM_COUNT) animReq = a;
    } else if (c == 'M' && len > 1) {
      int m = parseNum(data, len);
      if (m >= 0 && m < AUTO_MODE_COUNT) modeReq = m;
    } else if (c == 'Y' && len > 1) {                  // vitrine: Y0 / Y1 / Y2
      int v = parseNum(data, len);
      if (v >= 0 && v <= 2) showReq = v;
    } else if (c == 'K' && len > 1) {                  // calibração
      char buf[16] = {0};
      memcpy(buf, data + 1, min(len - 1, sizeof(buf) - 1));
      const char op = buf[0], side = buf[1];
      const int v = atoi(buf + 2);
      if (op == 'T' && (side == 'L' || side == 'R')) {          // "KTL-12": ponto de parada (µs)
        (side == 'L' ? leftTrim : rightTrim) = constrain(v, -150, 150);
      } else if (op == 'D' && (side == 'L' || side == 'R')) {   // "KDR-1": sentido
        (side == 'L' ? leftDir : rightDir) = (v < 0) ? -1 : 1;
      } else if (op == 'W' && (side == 'L' || side == 'R' || side == 'A')) {  // "KWA40": teste (segurar)
        calWheel = side;
        calSpeed = constrain(v, -100, 100);
        currentCmd = CAL;
        lastCmdMs = millis(); lastManualMs = lastCmdMs;
      } else if (op == 'S') {
        calSaveReq = true;
      } else if (op == 'Z') {
        calResetReq = true;
      }
    } else if (c == 'J' && len > 1) {                  // "J<x>,<y>"
      char buf[16] = {0};
      memcpy(buf, data + 1, min(len - 1, sizeof(buf) - 1));
      int x = 0, y = 0;
      if (sscanf(buf, "%d,%d", &x, &y) == 2) {
        joyX = constrain(x, -100, 100);
        joyY = constrain(y, -100, 100);
        currentCmd = JOY;
        lastCmdMs = millis();
        if (x != 0 || y != 0) { lastMoveMs = lastCmdMs; lastManualMs = lastCmdMs; }
      }
    } else if (strchr("SFBLRW", c)) {
      currentCmd = c;
      lastCmdMs = millis();
      if (c != STOP) { lastMoveMs = lastCmdMs; lastManualMs = lastCmdMs; }
    }
  }
}

// ---------------- Wi-Fi ----------------
DNSServer dns;
Preferences prefs;

// ---- calibração das rodas (salva na memória) ----
void loadCalib() {
  prefs.begin("cozmo", true);
  leftTrim  = prefs.getInt("ltrim", DEF_LEFT_TRIM);
  rightTrim = prefs.getInt("rtrim", DEF_RIGHT_TRIM);
  leftDir   = prefs.getInt("ldir",  DEF_LEFT_DIR);
  rightDir  = prefs.getInt("rdir",  DEF_RIGHT_DIR);
  prefs.end();
}
void saveCalib() {
  prefs.begin("cozmo", false);
  prefs.putInt("ltrim", leftTrim);  prefs.putInt("rtrim", rightTrim);
  prefs.putInt("ldir",  leftDir);   prefs.putInt("rdir",  rightDir);
  prefs.end();
}
String calibJson() {
  return "{\"ltrim\":" + String(leftTrim) + ",\"rtrim\":" + String(rightTrim) +
         ",\"ldir\":" + String(leftDir) + ",\"rdir\":" + String(rightDir) + "}";
}
String staSsid, staPass;        // rede de casa em uso (salva pela página ou do código)
String failSsid;                // rede que falhou no último boot (para avisar na página)
bool homeLost = false;          // a rede de casa caiu com ele ligado e ele foi para a rede própria
RTC_DATA_ATTR uint8_t homeRetries = 0;   // sobrevive ao reinício (zera ao desligar da energia)

void radioTweaks() {
  WiFi.setTxPower(WIFI_TX_POWER);
  WiFi.setSleep(false);         // sem economia de energia no rádio: responde mais rápido e cai menos
}
volatile uint32_t restartAt = 0;

// ---- busca de redes (feita no loop, a página só pede e consulta) ----
volatile bool scanReq = false;
volatile uint8_t scanState = 0;  // 0 = parado, 1 = buscando, 2 = pronto
String scanJson = "[]";

String jsonEsc(const String& in) {
  String o;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in.c_str()[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}

void loadWifiConfig() {
  prefs.begin("cozmo", true);
  staSsid  = prefs.getString("ssid", "");
  staPass  = prefs.getString("pass", "");
  failSsid = prefs.getString("fail", "");
  prefs.end();
  if (staSsid.length() == 0 && strlen(WIFI_SSID) > 0 && strcmp(WIFI_SSID, "WIFI NAME") != 0) {
    staSsid = WIFI_SSID; staPass = WIFI_PASS;           // nada salvo: usa o que está no código
  }
}

void setFail(const String& ssid) {
  prefs.begin("cozmo", false);
  if (ssid.length()) prefs.putString("fail", ssid); else prefs.remove("fail");
  prefs.end();
  failSsid = ssid;
}

void startAP() {
  apMode = true;
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL);
  radioTweaks();
  ipText = WiFi.softAPIP().toString();                 // 192.168.4.1
  // Responde todos os nomes com o IP do robô: o celular "acha" que tem internet
  // e não volta sozinho para outra rede
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
}

void startNetwork() {
  WiFi.persistent(false);                              // quem guarda a rede é o Preferences, não o Wi-Fi
  WiFi.setHostname(HOSTNAME);
  loadWifiConfig();

  if (staSsid.length() > 0) {
    WiFi.mode(WIFI_STA);
    radioTweaks();
    WiFi.setAutoReconnect(true);
    WiFi.begin(staSsid.c_str(), staPass.c_str());
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
      char seg[24];
      snprintf(seg, sizeof(seg), "%us restantes", (unsigned)((WIFI_TIMEOUT_MS - (millis() - t0)) / 1000));
      showMessage("Conectando ao Wi-Fi", staSsid.c_str(), seg);
      delay(250);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    ipText = WiFi.localIP().toString();
    homeRetries = 0;
    if (failSsid.length()) setFail("");
  } else {
    if (staSsid.length()) setFail(staSsid);            // avisa na página que não conectou
    // Desiste da rede de casa de vez: se o rádio continuasse procurando,
    // trocaria de canal e derrubaria a rede própria
    startAP();
  }
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  Serial.printf("%s  IP: %s\n", apMode ? "Modo AP" : "Conectado", ipText.c_str());
}

// Rede de casa caiu com ele ligado -> vai para a rede própria.
// Na rede própria, de tempos em tempos (e só se ninguém estiver conectado nela)
// procura a rede de casa; se ela voltou, reinicia para entrar nela.
void updateWifiFallback(uint32_t now) {
  static uint32_t lostSince = 0, lastTry = 0;
  static bool homeScan = false;

  if (!apMode) {
    if (WiFi.status() == WL_CONNECTED) { lostSince = 0; return; }
    if (lostSince == 0) lostSince = now;
    if (now - lostSince > HOME_LOST_MS) {
      homeLost = true;
      startAP();
      clientSeen = false;                               // volta a mostrar no display como conectar
      lastTry = now;
    }
    return;
  }

  if (staSsid.length() == 0 || homeRetries >= HOME_MAX_RETRY) return;
  if (!homeScan) {
    if (now - lastTry < HOME_RETRY_MS || scanState == 1 || scanReq) return;
    if (WiFi.softAPgetStationNum() > 0) { lastTry = now; return; }   // alguém usando a rede própria: não atrapalha
    lastTry = now;
    WiFi.mode(WIFI_AP_STA);
    WiFi.scanNetworks(true);
    homeScan = true;
    return;
  }
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  bool found = false;
  for (int i = 0; i < n; i++) if (WiFi.SSID(i) == staSsid) found = true;
  WiFi.scanDelete();
  WiFi.mode(WIFI_AP);
  homeScan = false;
  if (found) {                                          // a rede de casa voltou: reinicia para entrar nela
    homeRetries++;
    restartAt = now + 500;
  }
}

void updateScan() {
  if (scanReq && scanState != 1) {
    scanReq = false;
    if (apMode) WiFi.mode(WIFI_AP_STA);                // a busca precisa do modo cliente ligado
    WiFi.scanNetworks(true);                           // assíncrona: não trava o rosto
    scanState = 1;
  }
  if (scanState == 1) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return;
    String j = "[";
    int count = 0;
    // ordena por sinal e tira nomes repetidos (vários roteadores da mesma rede)
    for (int k = 0; k < n && count < 15; k++) {
      int best = -1;
      for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i).length() == 0) continue;
        bool used = false;
        for (int u = 0; u < n && !used; u++) used = (u != i && WiFi.SSID(u) == WiFi.SSID(i) && WiFi.RSSI(u) > WiFi.RSSI(i));
        if (used) continue;
        if (j.indexOf("\"" + jsonEsc(WiFi.SSID(i)) + "\"") >= 0) continue;
        if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
      }
      if (best < 0) break;
      if (count++) j += ',';
      j += "{\"ssid\":\"" + jsonEsc(WiFi.SSID(best)) + "\",\"rssi\":" + String(WiFi.RSSI(best)) +
           ",\"lock\":" + String(WiFi.encryptionType(best) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    scanJson = j + "]";
    WiFi.scanDelete();
    if (apMode) WiFi.mode(WIFI_AP);
    scanState = 2;
  }
}

String wifiStatusJson() {
  String j = "{\"mode\":\"";
  j += apMode ? "ap" : "sta";
  j += "\",\"ssid\":\"" + jsonEsc(apMode ? String(AP_SSID) : staSsid) + "\"";
  j += ",\"ip\":\"" + ipText + "\",\"host\":\"" + String(HOSTNAME) + ".local\"";
  j += ",\"fail\":\"" + jsonEsc(failSsid) + "\"";
  j += ",\"lost\":\"" + String(homeLost ? jsonEsc(staSsid) : String("")) + "\"}";
  return j;
}

// ---------------- Setup / Loop ----------------
void setup() {
  // Primeira coisa do boot: servos sem pulso (parados) até o 1º comando de movimento
  servosRelease();
  loadCalib();

  Serial.begin(115200);
  randomSeed(esp_random());

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED não encontrado");
  display.setTextColor(SSD1306_WHITE);
  eyes.begin(&display, SCREEN_WIDTH, SCREEN_HEIGHT);

  startNetwork();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", INDEX_HTML); });
  server.on("/faces", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "application/json", facesJson()); });

  server.on("/calib", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "application/json", calibJson()); });

  // ---- Wi-Fi pela página ----
  server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "application/json", wifiStatusJson()); });
  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (r->hasParam("start")) { scanReq = true; scanState = 1; }
    if (scanState == 1 || scanReq) r->send(200, "application/json", "{\"scanning\":true}");
    else r->send(200, "application/json", "{\"scanning\":false,\"nets\":" + scanJson + "}");
  });
  server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest* r) {
    String ssid = r->hasParam("ssid", true) ? r->getParam("ssid", true)->value() : "";
    String pass = r->hasParam("pass", true) ? r->getParam("pass", true)->value() : "";
    if (ssid.length() < 1 || ssid.length() > 32) { r->send(400, "text/plain", "Nome da rede inválido"); return; }
    if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) { r->send(400, "text/plain", "A senha precisa ter de 8 a 63 caracteres"); return; }
    prefs.begin("cozmo", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.remove("fail");
    prefs.end();
    r->send(200, "text/plain", "ok");
    restartAt = millis() + 1500;                       // reinicia já conectando na rede nova
  });
  server.on("/wifi/forget", HTTP_POST, [](AsyncWebServerRequest* r) {
    prefs.begin("cozmo", false);
    prefs.remove("ssid"); prefs.remove("pass"); prefs.remove("fail");
    prefs.end();
    r->send(200, "text/plain", "ok");
    restartAt = millis() + 1500;
  });
  // Testes de conectividade dos celulares: responder "tem internet" evita que
  // o Android/iPhone abandone a rede do robô por ela não ter internet
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(204); });     // Android
  server.on("/gen_204",      HTTP_GET, [](AsyncWebServerRequest* r) { r->send(204); });     // Android
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* r) {                // iPhone/Mac
    r->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* r) {                    // Windows
    r->send(200, "text/plain", "Microsoft Connect Test");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/plain", "Microsoft NCSI"); });
  server.onNotFound([](AsyncWebServerRequest* r) { r->redirect("http://" + ipText + "/"); });
  server.begin();
}

void loop() {
  const uint32_t now = millis();
  updateDrive(now);

  if (!clientSeen) {                                  // tela de status até o 1º acesso
    static uint32_t lastMsg = 0;
    if (now - lastMsg > 1000) {
      lastMsg = now;
      String url = "http://" + ipText;
      if (apMode) showMessage(("Wi-Fi: " + String(AP_SSID)).c_str(), ("Senha: " + String(AP_PASS)).c_str(), url.c_str(),
                              homeLost ? "Rede de casa caiu" : failSsid.length() ? "Falhou a rede de casa" : "Configure na pagina");
      else        showMessage("Acesse:", url.c_str(), "ou http://cozmo.local");
    }
  } else {
    updateDisplay(now);
  }

  static uint32_t lastClean = 0;
  if (now - lastClean > 2000) { ws.cleanupClients(); lastClean = now; }

  if (apMode) dns.processNextRequest();
  updateScan();
  updateWifiFallback(now);

  if (calSaveReq)  { calSaveReq = false; saveCalib(); }       // grava fora da task do servidor
  if (calResetReq) {
    calResetReq = false;
    leftTrim = DEF_LEFT_TRIM; rightTrim = DEF_RIGHT_TRIM; leftDir = DEF_LEFT_DIR; rightDir = DEF_RIGHT_DIR;
    saveCalib();
  }

  if (restartAt && (int32_t)(now - restartAt) >= 0) {   // depois de salvar o Wi-Fi
    showMessage("Reiniciando...", "", "Conectando na rede", "nova");
    delay(300);
    ESP.restart();
  }
}
