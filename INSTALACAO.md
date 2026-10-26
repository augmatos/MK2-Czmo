# Instalação e montagem — Cozmo-MK2

Guia de montagem, instalação do software, primeiro uso e solução de problemas.
Baseado nas instruções do [repositório original](https://github.com/nturmose21/MK2-Czmo) e atualizado para a versão revisada desta pasta.

---

## 1. Lista de materiais

| Item | Observação |
|------|------------|
| **ESP32-C3 SuperMini** | Controlador e host do Wi-Fi |
| **Display OLED SSD1306 0.96"** | 128×64, I²C, endereço **0x3C**. Modelo amarelo/azul (faixa amarela no topo) é o recomendado; funciona também em uma cor |
| **2× servos de rotação contínua** | SG90 ou MG90S (um para cada roda) |
| **Bateria Li-ion / LiPo 3,7 V** | Com módulo **TP4056** para carregar com segurança |
| **Fios, protoboard ou placa** | Ou um chassi próprio |
| **Peças impressas em 3D** | Arquivos em `hardware/3D_Models/` |

---

## 2. Ligações

| Pino ESP32-C3 | Conexão |
|---------------|---------|
| **GPIO 6** | Sinal do servo **esquerdo** |
| **GPIO 7** | Sinal do servo **direito** |
| **GPIO 8** | SDA do OLED |
| **GPIO 9** | SCL do OLED |
| **3,3 V / GND** | Alimentação do OLED e dos sinais |

> Os pinos estão definidos no início de `Cozmo-MK2.ino`:
> `const int LEFT_PIN = 6, RIGHT_PIN = 7, SDA_PIN = 8, SCL_PIN = 9;`
> Se a sua placa for ligada em outros pinos, altere essa linha.

**Alimentação:** os servos puxam bastante corrente nos picos. Use bateria com boa capacidade e fios grossos, e não alimente os servos pelo 3,3 V da placa.

---

## 3. Montagem mecânica

Os arquivos estão em `hardware/`. Imprima:

- 2× `Servo_Gear_Wheel.stl` (engrenagem da roda)
- 2× `Screw_Wheel.stl` (roda)
- 2× `Servo_Bracket.stl` (suporte do servo)
- 2× `Wheel_Spacer.stl` (espaçador)

O molde `CARDBOARD TEMPLATE.png` serve para recortar o chassi em papelão antes de montar a versão definitiva.

Fixe cada servo no suporte, encaixe a engrenagem no eixo e a roda na engrenagem. Use o espaçador entre a roda e o suporte para dar folga.

---

## 4. Software: Arduino IDE

### 4.1 Instalar a placa ESP32
1. Em **File → Preferences**, adicione a URL de gerenciadores de placas do Espressif:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Em **Tools → Board → Boards Manager**, instale **esp32 by Espressif Systems** (testado com a versão **3.3.10**).
3. Selecione a placa **ESP32C3 Dev Module**.

### 4.2 Instalar as bibliotecas
Abra **Sketch → Include Library → Manage Libraries** e instale:

| Biblioteca | Autor / fonte |
|------------|---------------|
| **ESP32Servo** | Kevin Harrington / John K. Bennett |
| **Adafruit GFX Library** | Adafruit |
| **Adafruit SSD1306** | Adafruit |
| **ESPAsyncWebServer** (versão ESP32Async) | ESP32Async |
| **AsyncTCP** (versão ESP32Async) | ESP32Async |

> `DNSServer`, `WiFi`, `ESPmDNS`, `Preferences` e `Wire` já vêm com o core do ESP32.
>
> Use as versões **ESP32Async** de `ESPAsyncWebServer` e `AsyncTCP`. As versões antigas não compilam com este código.

### 4.3 Configurar a placa para upload
Em **Tools**, ajuste:

| Opção | Valor |
|-------|-------|
| **Partition Scheme** | **Minimal SPIFFS (1.9MB APP with OTA)** |
| **USB CDC On Boot** | **Enabled** |

> **Importante:** com o esquema padrão de 4 MB o sketch não cabe (aparece `Sketch too big`). A versão revisada ocupa cerca de **69%** da partição Minimal SPIFFS.

### 4.4 Enviar o programa
1. Abra `Cozmo-MK2/Cozmo-MK2.ino`. **Todos os arquivos `.h` precisam estar na mesma pasta.**
2. Conecte a placa por USB.
3. Clique em **Upload**.

> **Não abra os arquivos soltos fora da pasta `Cozmo-MK2/`.** Se faltar algum `.h`, o compilador acusa erro.

---

## 5. Primeiro uso

### 5.1 Rede própria do robô
Depois de ligar, o robô cria uma rede Wi-Fi própria:

- **Nome:** `Cozmo-MK2`
- **Senha:** `cozmo1234`
- **Endereço da página:** `http://192.168.4.1`

O display mostra o nome, a senha e o endereço.

### 5.2 Conectar o robô à sua rede
1. Conecte o celular na rede `Cozmo-MK2`.
2. Abra `http://192.168.4.1`.
3. Use o painel **Wi-Fi** da página: clique em **Buscar**, escolha sua rede, digite a senha e clique em **Salvar**.
4. O robô se conecta e mostra o **endereço IP** no display.

Depois disso, o celular precisa estar na **mesma rede** do robô para acessar a página pelo IP mostrado no display. Se o Wi-Fi cair, o robô volta a criar a própria rede e tenta reconectar sozinho.

> **Dica:** nem sempre o celular abre a página pelo nome. Use o **número do IP** que aparece no display.

### 5.3 Controles da página
- **Joystick** e botões de segurar: dirigem o robô.
- **Emoções e animações:** toque em uma emoção para trocá-la.
- **Andar sozinho:** escolha entre **Inquieto** (mexidinhas curtas) e **Curioso** (explora um pouco).
- **Calibrar rodas:** ajusta o ponto de parada de cada servo (em microssegundos, com botões de ajuste fino) e o sentido de giro. Se uma roda girar devagar quando deveria estar parada, mova o ajuste dela até parar. Clique em **Salvar calibração** para guardar; o valor fica na memória mesmo depois de desligar.
- **Modo vitrine:** roteiro automático com todas as emoções, para demonstração ou vídeo. Pode ser feito com ou sem movimento das rodas.

---

## 6. Criar suas próprias animações (GIF → bitmap)

O conversor `Cozmo-MK2/tools/gif2header.py` gera um `anim_<nome>.h` pronto para a pasta do sketch.

```bash
pip install pillow

# a partir de um GIF animado
python tools/gif2header.py meu_gif.gif --name minha_anim

# a partir de uma pasta com PNG/JPG numerados
python tools/gif2header.py frames/ --name minha_anim --fps 20

# a partir do header do image2cpp (formato do Emotions.h original)
python tools/gif2header.py Emotions.h --name shush --fps 30
```

Depois de gerar o arquivo:

1. Copie `anim_minha_anim.h` para a pasta `Cozmo-MK2/`.
2. Adicione `#include "anim_minha_anim.h"` no `Cozmo-MK2.ino`.
3. Registre a animação na lista `ANIMS[]` (ou `MICROS[]`, para micro-expressões) no mesmo arquivo.
4. Se quiser que ela aconteça como reação, associe-a em `REACTION_ANIM[]`.

**Dicas:**
- Frames repetidos são guardados uma vez só. Cada frame único ocupa 1 KB de memória do programa, então animações longas pesam.
- Para preto e branco limpo, use GIF em alto contraste. Use `--threshold` (0-255) para ajustar o corte entre preto e branco, `--invert` se a animação sair invertida e `--dither` para fotos.
- `--stretch` estica a imagem para 128×64 em vez de manter a proporção.
- O display é **128×64**. A faixa amarela são as 16 linhas de cima: mantenha os olhos abaixo dela, na parte azul.

---

## 7. Solução de problemas

| Sintoma | Causa provável | O que fazer |
|---------|----------------|-------------|
| `'asyncfunction' does not name a type` | Algum arquivo `.h` com JavaScript está sendo lido pelo IDE como código | Mantenha a página em `WebPage.h`, como está nesta pasta |
| `Sketch too big` / 104% | Esquema de partição padrão | Use **Minimal SPIFFS (1.9MB APP with OTA)** |
| Página não abre na rede de casa | Celular usando outro endereço | Use o IP mostrado no display; desligue dados móveis temporariamente |
| Celular perde a conexão ao desligar o modem | Rede de casa caiu e o robô voltou para a própria rede | Espere alguns segundos; o robô volta a procurar a rede salva sozinho |
| Robô vira para o lado errado nos olhos | Display montado de outro jeito | Em `Eyes.h`, mude `LOOK_MIRROR_X` para `false` |
| Roda gira devagar parada | Ponto de parada do servo diferente | Use **Calibrar rodas** na página |
| Display vazio | Endereço I²C diferente ou fio solto | Confirme o endereço 0x3C e as ligações SDA/SCL |

---

## 8. Próximos passos sugeridos

- Atualização de firmware pelo Wi-Fi (OTA)
- Relógio com NTP
- Preferências salvas (modo, velocidade e humor) na memória do ESP32
- Controle por giroscópio, se for adicionar um sensor

Essas ideias ainda não estão implementadas.
