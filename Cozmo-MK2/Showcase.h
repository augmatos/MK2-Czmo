// =====================================================================
//  Showcase.h — modo vitrine
//
//  Um "roteiro" que passa por todas as emoções e animações, com
//  movimentos curtos no lugar, em loop. Bom para gravar vídeo ou
//  mostrar o robô para alguém sem precisar mexer no controle.
//
//  Para mudar o roteiro, edite SHOW_SCRIPT: cada linha é um passo.
// =====================================================================
#pragma once
#include <Arduino.h>
#include "Eyes.h"

struct ShowStep {
  int8_t      mood;      // emoção dos olhos (-1 = mantém a anterior)
  const char* anim;      // animação em bitmap pelo nome (nullptr = nenhuma)
  float       l, r;      // rodas (-1..1); só giros no lugar e balancinhos
  bool        look;      // true = olha para (lx, ly)
  float       lx, ly;
  uint16_t    ms;        // duração do passo (com animação: tempo extra depois dela)
};

// Atalhos para escrever o roteiro
#define S_FACE(m, ms)           { m,  nullptr, 0, 0, false, 0, 0, ms }
#define S_LOOK(m, x, y, ms)     { m,  nullptr, 0, 0, true,  x, y, ms }
#define S_ANIM(name, ms)        { -1, name,    0, 0, false, 0, 0, ms }
#define S_MOVE(m, l, r, ms)     { m,  nullptr, l, r, false, 0, 0, ms }
#define S_TURN(m, l, r, x, ms)  { m,  nullptr, l, r, true,  x, 0, ms }

const ShowStep SHOW_SCRIPT[] = {
  // --- curioso, olhando em volta ---
  S_LOOK(NEUTRAL,  0,    0,    1200),
  S_LOOK(NEUTRAL, -1,    0,     900),
  S_LOOK(NEUTRAL,  1,   -0.3f,  900),
  S_ANIM("micro_olhar_lateral",  200),
  S_LOOK(NEUTRAL,  0,    0,     600),
  // --- feliz e apaixonado ---
  S_FACE(HAPPY,                 1500),
  S_ANIM("micro_sorriso",        200),
  S_ANIM("apaixonados",          400),
  // --- surpresa ---
  S_FACE(SURPRISED,              900),
  S_ANIM("surpresa",             400),
  // --- desconfiado: vira para um lado e para o outro ---
  S_FACE(SUSPICIOUS,             700),
  S_TURN(SUSPICIOUS, -1,  1, -1,  250),
  S_LOOK(SUSPICIOUS, -1, 0,       700),
  S_TURN(SUSPICIOUS,  1, -1,  1,  500),
  S_LOOK(SUSPICIOUS,  1, 0,       700),
  S_TURN(SUSPICIOUS, -1,  1,  0,  250),
  S_ANIM("micro_desconfianca",   300),
  // --- bravo ---
  S_FACE(ANGRY,                 1300),
  S_ANIM("raiva",                400),
  // --- triste ---
  S_LOOK(SAD, 0, 0.5f,          1500),
  S_ANIM("micro_suspiro",        400),
  // --- medo ---
  S_ANIM("medo",                 300),
  S_MOVE(SURPRISED, -1, -1,      150),
  S_FACE(SURPRISED,              500),
  // --- dancinha e tontura ---
  S_FACE(HAPPY,                  400),
  S_MOVE(HAPPY,  1, -1,          120), S_MOVE(HAPPY, -1, 1, 120),
  S_MOVE(HAPPY,  1, -1,          120), S_MOVE(HAPPY, -1, 1, 120),
  S_MOVE(HAPPY,  1, -1,          120), S_MOVE(HAPPY, -1, 1, 120),
  S_MOVE(HAPPY,  1, -1,          120), S_MOVE(HAPPY, -1, 1, 120),
  S_FACE(DIZZY,                 2800),
  // --- confuso e cansado ---
  S_ANIM("confuso",              300),
  S_ANIM("cansado",              300),
  // --- bocejo, sono e acorda ---
  S_ANIM("bocejo",               100),
  S_FACE(SLEEPY,                1200),
  S_ANIM("sono",                 300),
  S_ANIM("micro_sobressalto",    200),
  S_FACE(NEUTRAL,                500),
  // --- final: "shhh" e uma piscadela ---
  S_ANIM("shush",                300),
  S_ANIM("micro_piscar_um",      1500),
};
const uint8_t SHOW_LEN = sizeof(SHOW_SCRIPT) / sizeof(SHOW_SCRIPT[0]);

class Showcase {
public:
  // Saída lida pelo sketch
  int8_t      mood = NEUTRAL;
  float       wheelL = 0, wheelR = 0;
  bool        hasLook = false;
  float       lookX = 0, lookY = 0;
  const char* wantAnim = nullptr;   // o sketch toca e zera

  bool motion = true;               // false = só rosto (para filmar em cima da mesa)

  // animMs: função do sketch que diz quanto tempo dura uma animação pelo nome
  void start(uint32_t now, bool withMotion) { active = true; motion = withMotion; idx = 0; begin(now); }
  void stop() { active = false; wheelL = wheelR = 0; hasLook = false; wantAnim = nullptr; }
  bool isActive() const { return active; }

  void update(uint32_t now, uint32_t (*animMs)(const char*)) {
    if (!active) return;
    if (now - stepStart >= stepLen(animMs)) {
      idx = (idx + 1) % SHOW_LEN;                          // fim do roteiro: recomeça
      begin(now);
    }
    const ShowStep& s = SHOW_SCRIPT[idx];
    wheelL = motion ? s.l * 0.6f : 0;
    wheelR = motion ? s.r * 0.6f : 0;
  }

  // Pausa (controle manual em uso): retoma do mesmo passo depois
  void hold(uint32_t now) { if (active) { stepStart = now; wheelL = wheelR = 0; } }

private:
  bool active = false;
  uint8_t idx = 0;
  uint32_t stepStart = 0;

  void begin(uint32_t now) {
    const ShowStep& s = SHOW_SCRIPT[idx];
    stepStart = now;
    if (s.mood >= 0) mood = s.mood;
    hasLook = s.look;
    lookX = s.lx; lookY = s.ly;
    if (s.anim) wantAnim = s.anim;
  }
  uint32_t stepLen(uint32_t (*animMs)(const char*)) const {
    const ShowStep& s = SHOW_SCRIPT[idx];
    return s.ms + (s.anim && animMs ? animMs(s.anim) : 0);
  }
};
