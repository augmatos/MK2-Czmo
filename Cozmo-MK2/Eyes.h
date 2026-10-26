// =====================================================================
//  Eyes.h — olhos procedurais estilo Cozmo, ajustados para OLED bicolor
//
//  Display "yellow/blue": as 16 linhas de cima são amarelas e as 48 de
//  baixo, azuis. Os olhos ficam sempre na área azul; a faixa amarela é
//  usada de propósito como SOBRANCELHA nas emoções em que isso combina.
//  Display de uma cor só? Coloque TOP_ZONE = 0 (sobrancelhas são desligadas).
// =====================================================================
#pragma once
#include <Adafruit_GFX.h>

const int16_t TOP_ZONE = 16;   // linhas da faixa amarela (0 = display de uma cor)
const int16_t ZONE_GAP = 1;    // folga entre as faixas (há uma linha "morta" entre elas)

// Geometria igual à dos GIFs (olhos 28x24 com pupila, topo na linha 17, espaço para a boca embaixo).
// Assim as microexpressões em bitmap entram e saem sem "pulo" no rosto.
const float EYE_CY  = 28.5f;   // centro vertical dos olhos
const bool  PUPILS  = true;    // desenha pupila com brilho (segue o olhar)
// Inverte esquerda/direita do olhar. true = quando ele vira para a direita, os olhos vão
// para a direita de quem está olhando para a tela. Se ficar trocado, mude para false.
const bool  LOOK_MIRROR_X = true;

// ---- 1) Para criar uma emoção nova: acrescente um nome aqui... ----
enum Emotion : uint8_t {
  NEUTRAL, HAPPY, SAD, ANGRY, SURPRISED, SLEEPY, SUSPICIOUS, DIZZY,
  EMOTION_COUNT
};

struct Face {
  // --- olhos (área azul) ---
  float w, h;     // largura e altura de cada olho (px)
  float r;        // arredondamento dos cantos (px)
  float gap;      // distância entre os olhos (px)
  float lidTop;   // pálpebra de cima: fração da altura coberta (0..1)
  float tilt;     // inclinação da pálpebra: + = lado de dentro desce (bravo) | - = lado de fora desce (triste)
  float lidBot;   // pálpebra de baixo curva: fração coberta (olho "sorrindo")
  float offY;     // deslocamento vertical dos olhos (px, + = para baixo)
  // --- sobrancelhas (faixa amarela) ---
  float brow;     // 1 = mostra, 0 = esconde
  float browY;    // altura da sobrancelha dentro da faixa amarela (0 = topo, 15 = base)
  float browTilt; // + = ponta de dentro mais baixa (bravo) | - = ponta de dentro mais alta (preocupado)
  float browArc;  // curvatura para cima no meio (px)
  float browLift; // levanta só a sobrancelha esquerda (px) — desconfiado
  // --- especial ---
  float spiral;   // 1 = olhos viram espirais girando (tonto)
};

// ---- 2) ...e uma linha aqui, na mesma posição do enum ----
const Face FACES[EMOTION_COUNT] = {
  //  w    h    r  gap  lidTop  tilt lidBot offY | brow browY bTilt bArc bLift | spiral
  { 28,  24,  6, 22, 0.00,  0.00, 0.00,  0,     0,   10,    0,   0,   0,   0 },  // NEUTRAL    (igual aos GIFs)
  { 30,  22,  8, 20, 0.00,  0.00, 0.45,  1,     1,    9,    0,   3,   0,   0 },  // HAPPY      (sobrancelhas arqueadas)
  { 27,  21,  6, 24, 0.25, -0.35, 0.00,  3,     1,   10,   -6,   0,   0,   0 },  // SAD        (pontas de dentro sobem)
  { 28,  21,  5, 18, 0.25,  0.45, 0.00,  2,     1,    8,    8,   0,   0,   0 },  // ANGRY      (franzidas para dentro)
  { 32,  28,  8, 20, 0.00,  0.00, 0.00,  2,     1,    5,    0,   3,   0,   0 },  // SURPRISED  (lá em cima, arqueadas)
  { 28,  24,  6, 22, 0.62,  0.00, 0.00,  3,     0,   10,    0,   0,   0,   0 },  // SLEEPY     (sem sobrancelha)
  { 30,  20,  5, 20, 0.28,  0.10, 0.12,  1,     1,   10,    0,   0,   5,   0 },  // SUSPICIOUS (uma levantada)
  { 36,  34, 17, 16, 0.00,  0.00, 0.00, -2,     0,   10,    0,   0,   0,   1 },  // DIZZY      (olhos em espiral)
};

const char* const EMOTION_NAMES[EMOTION_COUNT] = {
  "Neutro", "Feliz", "Triste", "Bravo", "Surpreso", "Sono", "Desconfiado", "Tonto"
};

// =====================================================================
class Eyes {
public:
  float    tweenSpeed  = 0.25f;  // 0..1 — quanto se aproxima do alvo por frame
  uint16_t blinkMinMs  = 2000;
  uint16_t blinkMaxMs  = 6000;
  bool     idleLooking = true;   // olha em volta sozinho quando parado

  void begin(Adafruit_GFX* gfx, int16_t width = 128, int16_t height = 64) {
    g = gfx; W = width; H = height;
    cur = tgt = FACES[NEUTRAL];
    nextBlink = millis() + random(blinkMinMs, blinkMaxMs);
  }

  void setEmotion(Emotion e) {
    if (e < EMOTION_COUNT) { emotion = e; tgt = FACES[e]; }
  }
  Emotion getEmotion() const { return emotion; }

  // x, y entre -1 e 1 (x: -1 esquerda, +1 direita | y: -1 cima, +1 baixo)
  void lookAt(float x, float y) {
    lookTgtX = constrain(LOOK_MIRROR_X ? -x : x, -1.0f, 1.0f);
    lookTgtY = constrain(y, -1.0f, 1.0f);
    lastExternalLook = millis();
  }

  void blink() { blinkStart = millis(); }

  void update(uint32_t now) {
    nowMs = now;
    lerpFace(cur, tgt, tweenSpeed);
    lookX += (lookTgtX - lookX) * 0.35f;
    lookY += (lookTgtY - lookY) * 0.35f;

    if ((int32_t)(now - nextBlink) >= 0) {
      blinkStart = now;
      nextBlink = now + (random(5) == 0 ? 250 : random(blinkMinMs, blinkMaxMs));
    }

    if (idleLooking && now - lastExternalLook > 1500 && (int32_t)(now - nextSaccade) >= 0) {
      if (random(3) == 0) { lookTgtX = 0; lookTgtY = 0; }
      else { lookTgtX = random(-60, 61) / 100.0f; lookTgtY = random(-40, 41) / 100.0f; }
      nextSaccade = now + random(1200, 4000);
    }

    const uint32_t BLINK_MS = 160;
    uint32_t dt = now - blinkStart;
    if (dt < BLINK_MS) {
      float t = dt / (float)BLINK_MS;
      blinkF = max(0.08f, fabsf(2 * t - 1));
    } else blinkF = 1.0f;
  }

  void draw(uint16_t on = 1, uint16_t off = 0) {
    fg = on; bg = off;

    // Área azul onde os olhos podem ficar
    const float zTop = (TOP_ZONE > 0) ? TOP_ZONE + ZONE_GAP : 0;
    const float zBot = H - 1;
    const float zMid = (zTop + zBot) / 2;

    // O corpo do olho mexe pouco; quem "olha" é a pupila (como nos GIFs)
    const float cx = W / 2.0f + lookX * (PUPILS ? 0 : 14);
    const float pk = PUPILS ? 0.0f : 0.12f;              // perspectiva ao olhar de lado
    const float sL = 1.0f - pk * lookX;
    const float sR = 1.0f + pk * lookX;
    const float half = cur.gap / 2;
    const float xL = cx - half - cur.w * sL / 2;
    const float xR = cx + half + cur.w * sR / 2;

    // Altura e posição presas dentro da área azul (nunca invade o amarelo)
    const float hMax = zBot - zTop;
    const float hL = min(cur.h * sL, hMax), hR = min(cur.h * sR, hMax);
    const float hBig = max(hL, hR);
    float cy = (PUPILS ? EYE_CY : zMid) + cur.offY + lookY * (PUPILS ? 2 : 5);
    cy = constrain(cy, zTop + hBig / 2, zBot - hBig / 2);

    if (cur.spiral > 0.5f) {                            // modo tonto: espirais + boquinha
      drawDizzy(zTop, zBot);
      return;
    }

    drawEye(xL, cy, cur.w * sL, hL * blinkF, true);
    drawEye(xR, cy, cur.w * sR, hR * blinkF, false);

    // Sobrancelhas na faixa amarela (acompanham o olhar na horizontal)
    if (TOP_ZONE > 0 && cur.brow > 0.5f) {
      const float by = cur.browY - lookY * 1.5f + (1 - blinkF) * 1.5f;  // desce um pouco ao piscar
      drawBrow(xL, by - cur.browLift, cur.w * sL, true);
      drawBrow(xR, by,                cur.w * sR, false);
    }
  }

private:
  Adafruit_GFX* g = nullptr;
  int16_t W = 128, H = 64;
  uint16_t fg = 1, bg = 0;
  Emotion emotion = NEUTRAL;
  Face cur{}, tgt{};
  float lookX = 0, lookY = 0, lookTgtX = 0, lookTgtY = 0;
  float blinkF = 1;
  uint32_t nowMs = 0;
  uint32_t blinkStart = 0, nextBlink = 0, nextSaccade = 0, lastExternalLook = 0;


  // Olhos em espiral girando (sentidos opostos) + boquinha, com o rosto balançando
  void drawDizzy(float zTop, float zBot) {
    const float t     = nowMs / 1000.0f;
    const float sway  = sinf(t * 5.0f) * 4.0f;                    // balança de um lado pro outro
    const float bob   = sinf(t * 10.0f) * 1.0f;
    const float mouthH = 9;
    const float R     = min(18.0f, (zBot - zTop - mouthH - 4) / 2); // raio máximo da espiral
    const float cy    = zTop + R + 1 + bob;
    const float cx    = W / 2.0f + sway;
    const float dx    = R + cur.gap / 2 + 4;
    const float spin  = t * 5.2f;                                 // ~0,8 volta por segundo

    drawSpiral(cx - dx, cy, R,  spin);
    drawSpiral(cx + dx, cy, R, -spin + 3.14159f);

    // boquinha em "domo", centralizada embaixo dos olhos
    const int16_t mw = 17, mh = (int16_t)mouthH;
    int16_t mx = (int16_t)roundf(cx - mw / 2.0f);
    int16_t my = (int16_t)roundf(min(cy + R - 2, zBot - mh));
    g->fillRoundRect(mx, my, mw, mh, mh / 2, fg);
    g->fillRect(mx, my + mh - 3, mw, 3, bg);                      // base reta
  }

  // Espiral de Arquimedes com 2,5 voltas, traço de ~3 px
  void drawSpiral(float cx, float cy, float R, float phase) {
    const float TURNS = 2.5f, TH_MAX = TURNS * 6.28318f;
    const float a = R / TH_MAX;                                    // r = a * θ
    float th = 0.6f;
    while (th < TH_MAX) {
      float r = a * th;
      float ang = (phase >= 0 ? th : -th) + phase;                 // sentido de giro
      int16_t x = (int16_t)roundf(cx + r * cosf(ang));
      int16_t y = (int16_t)roundf(cy + r * sinf(ang));
      g->fillCircle(x, y, 1, fg);
      th += 0.9f / max(r, 1.5f);                                   // passo ~1 px ao longo da curva
    }
  }

  static void lerpFace(Face& a, const Face& b, float k) {
    float* pa = &a.w; const float* pb = &b.w;          // todos os campos são float, em sequência
    for (size_t i = 0; i < sizeof(Face) / sizeof(float); i++) pa[i] += (pb[i] - pa[i]) * k;
  }

  void drawEye(float cx, float cy, float w, float h, bool isLeft) {
    int16_t iw = max(2, (int)roundf(w));
    int16_t ih = max(2, (int)roundf(h));
    int16_t x  = (int16_t)roundf(cx - iw / 2.0f);
    int16_t y  = (int16_t)roundf(cy - ih / 2.0f);
    int16_t r  = min<int16_t>((int16_t)roundf(cur.r), min(iw, ih) / 2);

    g->fillRoundRect(x, y, iw, ih, r, fg);

    // Pupila escura com brilho, deslocada para onde ele olha.
    // Fica só na parte visível do olho (entre as pálpebras) para não "rasgar" o desenho.
    if (PUPILS && cur.lidBot < 0.3f) {                 // olho feliz em "^" fica sem pupila
      const float visTop = y + ih * (cur.lidTop + fabsf(cur.tilt) / 2) + 2;
      const float visBot = y + ih * (1 - cur.lidBot) - 2;
      int16_t pw = max<int16_t>(3, (int16_t)roundf(iw * 0.28f));
      int16_t ph = (int16_t)min(roundf(cur.h * 0.58f), visBot - visTop - 2);
      if (ph >= 3) {
        float midY = (visTop + visBot) / 2;
        float pcx = cx + lookX * (iw / 2.0f - pw / 2.0f - 3);
        float pcy = midY + lookY * max(0.0f, (visBot - visTop) / 2 - ph / 2.0f - 1);
        int16_t px = (int16_t)roundf(pcx - pw / 2.0f), py = (int16_t)roundf(pcy - ph / 2.0f);
        g->fillRoundRect(px, py, pw, ph, min(pw, ph) / 2, bg);
        if (ph >= 7) g->fillRect(px + 1, py + 2, 2, 2, fg);   // brilho
      }
    }

    // Pálpebra de cima: apaga tudo acima da reta (lado de fora -> lado de dentro)
    if (cur.lidTop > 0.01f || fabsf(cur.tilt) > 0.01f) {
      float base = y + cur.lidTop * ih;
      float yIn  = base + cur.tilt * ih / 2;
      float yOut = base - cur.tilt * ih / 2;
      int16_t xa = x - 1, xb = x + iw + 1;
      int16_t ya = (int16_t)roundf(isLeft ? yOut : yIn);
      int16_t yb = (int16_t)roundf(isLeft ? yIn : yOut);
      int16_t yMin = min(ya, yb), yMax = max(ya, yb);
      int16_t top = y - 1;
      if (yMin > top) g->fillRect(xa, top, xb - xa + 1, yMin - top + 1, bg);
      if (yMax > yMin) g->fillTriangle(xa, yMin, xb, yMin, (ya > yb) ? xa : xb, yMax, bg);
    }

    // Pálpebra de baixo curva (olho em forma de "^")
    if (cur.lidBot > 0.01f) {
      int16_t by  = (int16_t)roundf(y + ih - cur.lidBot * ih);
      int16_t pad = max<int16_t>(4, iw / 5);
      // pálpebra alta = topo em semicírculo: corta o olho em "^" sem deixar pontas soltas
      int16_t bw = iw + 2 * pad, bh = 2 * ih + 2;
      g->fillRoundRect(x - pad, by, bw, bh, min(bw, bh) / 2, bg);
    }
  }

  // Sobrancelha grossa (3 px), com inclinação e arco, presa dentro da faixa amarela
  void drawBrow(float cx, float y, float eyeW, bool isLeft) {
    const float len = eyeW * 0.85f;
    const float x0 = cx - len / 2, x1 = cx + len / 2;
    // ponta de dentro (lado do nariz) = direita do olho esquerdo, esquerda do olho direito
    const float yInner = y + cur.browTilt / 2, yOuter = y - cur.browTilt / 2;
    const float ya = isLeft ? yOuter : yInner;
    const float yb = isLeft ? yInner : yOuter;
    const int SEG = 8;
    int16_t px = 0, py = 0;
    for (int i = 0; i <= SEG; i++) {
      float t = i / (float)SEG;
      float xx = x0 + (x1 - x0) * t;
      float yy = ya + (yb - ya) * t - cur.browArc * 4 * t * (1 - t);
      int16_t ix = (int16_t)roundf(xx);
      int16_t iy = (int16_t)constrain(roundf(yy), 2.0f, (float)(TOP_ZONE - 4));
      if (i > 0) for (int d = -1; d <= 1; d++) g->drawLine(px, py + d, ix, iy + d, fg);
      px = ix; py = iy;
    }
  }
};
