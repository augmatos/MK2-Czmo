// BitmapAnim.h — formato das animações geradas por tools/gif2header.py
#pragma once
#include <Arduino.h>

struct BitmapAnim {
  const char*     name;
  const uint8_t (*frames)[1024];  // frames únicos 128x64 (1 bit/pixel)
  const uint16_t* seq;            // ordem de exibição (índices em frames)
  const uint16_t* ms;             // duração de cada passo da sequência
  uint16_t        len;            // passos na sequência
};
