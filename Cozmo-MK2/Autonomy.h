// =====================================================================
//  Autonomy.h — modos autônomos (inspirado no Mochan, mas sem delay())
//
//  O robô escolhe "comportamentos" sozinho: cada comportamento é uma
//  sequência curta de passos (rodas + emoção + para onde olhar).
//  Tudo não-bloqueante: o rosto e o Wi-Fi continuam funcionando.
//
//  Para criar um comportamento novo: escreva uma função planXxx() que
//  chama add(...) e sorteie ela em planFidget() ou planCurious().
// =====================================================================
#pragma once
#include <Arduino.h>
#include "Eyes.h"

enum AutoMode : uint8_t { AUTO_OFF, AUTO_FIDGET, AUTO_CURIOUS, AUTO_MODE_COUNT };
const char* const AUTO_MODE_NAMES[AUTO_MODE_COUNT] = { "Manual", "Inquieto", "Curioso" };

// Reações que tocam uma animação em bitmap (o sketch liga cada uma a um anim_*.h)
enum Reaction : int8_t { REACT_NONE = -1, REACT_SURPRISE, REACT_LOVE, REACT_ANGRY, REACT_SLEEP,
                         REACT_FEAR, REACT_YAWN, REACT_TIRED, REACT_CONFUSED };

// Segurança: o robô não tem sensores, então os movimentos são curtos e lentos
const float    AUTO_SPEED      = 0.6f;   // fração da velocidade máxima nos modos autônomos
const uint16_t AUTO_MAX_RUN_MS = 1200;   // maior trecho andando reto de uma vez

class Autonomy {
public:
  // Saída lida pelo sketch a cada loop
  float wheelL = 0, wheelR = 0;
  int   mood   = -1;          // -1 = não mexe na emoção
  bool  hasLook = false;
  float lookX = 0, lookY = 0;
  bool  wantBlink = false;    // pedido de piscada (o sketch zera depois de usar)
  int8_t wantReact = REACT_NONE; // pedido de animação (o sketch zera depois de usar)

  void setMode(AutoMode m) { mode = m; reset(millis()); }
  AutoMode getMode() const { return mode; }

  // Chamado enquanto o controle manual está em uso: cancela o que estava fazendo
  void reset(uint32_t now) {
    n = idx = 0;
    wheelL = wheelR = 0; mood = -1; hasLook = false;
    nextPlanAt = now + 1500;
  }

  void update(uint32_t now) {
    if (mode == AUTO_OFF) { wheelL = wheelR = 0; mood = -1; hasLook = false; return; }

    // Terminou a sequência? Espera um pouco e sorteia a próxima
    if (idx >= n) {
      wheelL = wheelR = 0; mood = -1; hasLook = false;
      if ((int32_t)(now - nextPlanAt) < 0) return;
      n = idx = 0;
      if (mode == AUTO_FIDGET) planFidget(); else planCurious();
      stepStart = now;
      if (n > 0 && steps[0].blink) wantBlink = true;
      if (n > 0 && steps[0].react != REACT_NONE) wantReact = steps[0].react;
    }

    // Avança os passos pelo tempo
    while (idx < n && now - stepStart >= steps[idx].ms) {
      stepStart += steps[idx].ms;
      if (++idx >= n) {
        nextPlanAt = now + (mode == AUTO_FIDGET ? random(3000, 8000) : random(1500, 5000));
        wheelL = wheelR = 0; mood = -1; hasLook = false;
        return;
      }
      if (steps[idx].blink) wantBlink = true;
      if (steps[idx].react != REACT_NONE) wantReact = steps[idx].react;
    }
    const Step& s = steps[idx];
    wheelL = s.l * AUTO_SPEED;
    wheelR = s.r * AUTO_SPEED;
    mood = s.mood;
    hasLook = s.look;
    lookX = s.lx; lookY = s.ly;
  }

  bool moving() const { return wheelL != 0 || wheelR != 0; }

private:
  struct Step { float l, r; uint16_t ms; int8_t mood; bool look; float lx, ly; bool blink; int8_t react; };
  static const uint8_t MAX_STEPS = 16;
  Step steps[MAX_STEPS];
  uint8_t n = 0, idx = 0;
  uint32_t stepStart = 0, nextPlanAt = 0;
  AutoMode mode = AUTO_OFF;

  // Adiciona um passo: rodas (-1..1), duração, emoção (-1 = mantém), olhar
  void add(float l, float r, uint16_t ms, int8_t mood = -1, bool look = false,
           float lx = 0, float ly = 0, bool blink = false) {
    if (n < MAX_STEPS) steps[n++] = { l, r, ms, mood, look, lx, ly, blink, REACT_NONE };
  }
  // Toca uma animação quando o último passo adicionado começar
  void react(Reaction r) { if (n > 0) steps[n - 1].react = r; }
  void pause(uint16_t ms, int8_t mood = -1) { add(0, 0, ms, mood); }
  void look(float lx, float ly, uint16_t ms, int8_t mood = -1) { add(0, 0, ms, mood, true, lx, ly); }

  // ---------------- Modo INQUIETO: mexidinhas curtas de vez em quando ----------------
  void planFidget() {
    const float side = random(2) ? 1 : -1;
    switch (random(5)) {
      case 0:  // vira um pouquinho para um lado, olhando para lá
        look(side, 0, 300);
        add(side, -side, random(80, 160), -1, true, side, 0);
        look(side, 0, 700);
        break;
      case 1:  // dá um passinho para frente
        look(0, -0.4f, 250);
        add(1, 1, random(100, 200), -1, true, 0, -0.4f);
        pause(500);
        break;
      case 2:  // recua um pouquinho, assustado
        add(-1, -1, random(80, 150), SURPRISED, true, 0, 0.3f);
        if (random(3) == 0) react(REACT_SURPRISE);           // às vezes faz a cara grande de susto
        pause(900, SURPRISED);
        break;
      case 3:  // tremidinha de animação
        for (int i = 0; i < 2; i++) { add(0.8f, -0.8f, 70, HAPPY); add(-0.8f, 0.8f, 70, HAPPY); }
        pause(600, HAPPY);
        break;
      default: // só olha e pisca, sem andar
        look(side * 0.8f, -0.3f, 900);
        add(0, 0, 400, -1, true, 0, 0, true);
        break;
    }
  }

  // ---------------- Modo CURIOSO: explora o ambiente ----------------
  void planCurious() {
    const float side = random(2) ? 1 : -1;
    switch (random(6)) {
      case 0:  // olha em volta: esquerda, direita, volta
        look(side, 0, 500);
        add(side, -side, random(250, 450), -1, true, side, 0);
        look(side, 0, 700);
        look(-side, 0, 400);
        add(-side, side, random(400, 700), -1, true, -side, 0);
        look(-side, 0, 600);
        add(side, -side, 250, -1, true, 0, 0);
        break;
      case 1:  // explora: anda para frente e para para observar
        look(0, -0.5f, 500);
        add(1, 1, random(500, AUTO_MAX_RUN_MS), -1, true, 0, -0.5f);
        look(side * 0.7f, -0.2f, 800);
        add(0, 0, 300, -1, true, 0, 0, true);
        break;
      case 2:  // "opa!": recua assustado e fica desconfiado
        pause(250, SURPRISED);
        react(random(2) ? REACT_SURPRISE : REACT_FEAR);       // susto: às vezes surpresa, às vezes medo
        add(-1, -1, random(250, 400), SURPRISED, true, 0, 0.5f);
        pause(1200, SUSPICIOUS);
        break;
      case 3:  // dancinha feliz (curta: não chega a ficar tonto)
        for (int i = 0; i < 4; i++) { add(0.9f, -0.9f, 110, HAPPY); add(-0.9f, 0.9f, 110, HAPPY); }
        pause(900, HAPPY);
        if (random(2) == 0) react(REACT_LOVE);                // metade das vezes termina apaixonado
        break;
      case 4:  // pensativo: para e olha para cima de lado
        look(side * 0.5f, -0.6f, 1800, SUSPICIOUS);
        add(0, 0, 300, -1, true, 0, 0, true);
        break;
      default: // vira para um lado novo e segue um pouco
        add(side, -side, random(300, 600), -1, true, side, 0);
        add(1, 1, random(400, 900), -1, true, 0, -0.3f);
        pause(500);
        break;
    }
  }
};
