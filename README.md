# MK2-Czmo — Cozmo-MK2 (versão revisada)

Robô pequeno com **ESP32-C3 SuperMini**, inspirado no Cozmo da Anki. Ele tem duas rodas com servos de rotação contínua, um display OLED amarelo/azul que mostra olhos expressivos e animações em GIF, e uma **página web de controle** hospedada no próprio ESP32 (funciona no celular ou no computador).

> **Esta pasta é uma derivação do projeto original [nturmose21/MK2-Czmo](https://github.com/nturmose21/MK2-Czmo).**
> Os créditos do projeto original estão em [CREDITS.md](CREDITS.md). O código original está preservado em [`original/`](original/) e os arquivos de impressão 3D em [`hardware/`](hardware/).

---

## O que tem de novo em relação ao original

- **Olhos procedurais** (`Eyes.h`): emoções são parâmetros, com transição suave, piscada automática, pupila com brilho e sobrancelhas na faixa amarela.
- **Animações em GIF** convertidas para bitmap (`anim_*.h`) e ligadas às reações do robô.
- **Micro-expressões** (piscadas, sorrisos, suspiros) entram sozinhas no meio das animações; não existe botão para elas.
- **Wi-Fi configurado pela própria página**, com rede própria de reserva (`Cozmo-MK2`) quando não há rede conhecida.
- **Joystick** na página, além dos botões de segurar.
- **Modos autônomos**: *Inquieto* e *Curioso*, sem `delay()`, então o rosto e o Wi-Fi continuam funcionando.
- **Calibração das rodas** pela página e **modo vitrine** (roteiro automático de emoções para demonstração).

O detalhe de cada mudança está em [CHANGELOG.md](CHANGELOG.md).

---

## Estrutura da pasta

```
MK2-Czmo/
├── README.md              ← você está aqui
├── INSTALACAO.md          ← montagem, bibliotecas, upload e primeiro uso
├── CREDITS.md             ← créditos, origem e observações de licença
├── CHANGELOG.md           ← o que mudou em relação ao original
├── Cozmo-MK2/             ← SKETCH (abra o Cozmo-MK2.ino no Arduino IDE)
│   ├── Cozmo-MK2.ino      ← programa principal
│   ├── Eyes.h             ← olhos procedurais e emoções
│   ├── Autonomy.h         ← modos Inquieto e Curioso
│   ├── Showcase.h         ← roteiro do modo vitrine
│   ├── WebPage.h          ← página de controle (HTML/CSS/JS)
│   ├── BitmapAnim.h       ← estrutura das animações em bitmap
│   ├── anim_*.h           ← animações convertidas (GIF → bitmap)
│   ├── gifs/              ← GIFs-fonte de algumas animações
│   └── tools/
│       └── gif2header.py  ← conversor GIF/PNG → anim_*.h
├── original/              ← código original do autor, sem alterações
│   ├── Cozmo-MK2.ino
│   ├── Emotions.h
│   └── README_ORIGINAL.md
└── hardware/              ← arquivos físicos do projeto original
    ├── 3D_Models/         ← STLs: rodas, suporte do servo, espaçadores
    └── CARDBOARD TEMPLATE.png  ← molde em papelão
```

---

## Começo rápido

1. Monte o hardware seguindo a tabela de pinos em [INSTALACAO.md](INSTALACAO.md).
2. Instale as bibliotecas listadas em [INSTALACAO.md](INSTALACAO.md).
3. No Arduino IDE, configure **Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)**.
4. Abra `Cozmo-MK2/Cozmo-MK2.ino` e envie para a placa.
5. Conecte no Wi-Fi `Cozmo-MK2` (senha `cozmo1234`) e acesse `http://192.168.4.1`, ou use o endereço que aparece no display.

Detalhes, solução de problemas e como criar suas próprias animações estão em [INSTALACAO.md](INSTALACAO.md).

---

## Licença

O repositório original **não traz arquivo de licença**. Leia [CREDITS.md](CREDITS.md) antes de publicar esta pasta em qualquer lugar.
