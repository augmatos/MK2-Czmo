# Changelog — em relação ao projeto original

Comparação entre a pasta `original/` (código do repositório nturmose21/MK2-Czmo) e a versão revisada em `Cozmo-MK2/`.

## Versão revisada

### Movimento
- Rodas com `writeMicroseconds` (parado = 1500 µs) e liberação do servo quando ocioso, para não girar sozinho.
- Mistura diferencial com expo e zona morta no joystick.
- Calibração das rodas pela página (valores salvos na memória do ESP32).
- Os movimentos autônomos são curtos e lentos (`AUTO_SPEED = 0.6`), porque o robô não tem sensores de obstáculo.
- Os servos ficam parados durante a conexão Wi-Fi.

### Olhos e expressões
- Olhos procedurais (`Eyes.h`) com emoções como parâmetros e transição suave.
- Piscada automática, pupila com brilho e movimento que segue o olhar.
- Olhos mantidos na parte azul do display; sobrancelhas usam a faixa amarela.
- Emoção de espiral (`DIZZY`).
- Olhar espelhado corrigido com a constante `LOOK_MIRROR_X`.

### Animações
- Animações em GIF convertidas para bitmap (`anim_*.h`), com o conversor `tools/gif2header.py`.
- Reações ligadas a eventos: susto, medo, amor, raiva, sono, bocejo, cansaço e confusão (quando o Wi-Fi cai).
- Micro-expressões (piscadas, sorriso, suspiro, desconfiança, sobressalto) entram sozinhas no meio das animações. Não há botão para acioná-las.

### Página web e Wi-Fi
- Página movida para `WebPage.h`, sem JavaScript no `.ino`. Isso evita o erro `'asyncfunction' does not name a type` no Arduino IDE.
- Configuração de Wi-Fi pela própria página: buscar redes, salvar e esquecer.
- Rede própria de reserva (`Cozmo-MK2`), com nova busca e retorno à rede de casa.
- Potência do Wi-Fi reduzida (8,5 dBm) para melhorar a estabilidade.
- Página com rolagem no celular (apenas o joystick e os botões de segurar bloqueiam o gesto de rolar).
- Joystick na página, além dos botões de segurar.

### Modos
- **Manual:** controle pela página.
- **Inquieto:** mexidinhas curtas de vez em quando.
- **Curioso:** explora um pouco e observa o ambiente.
- **Modo vitrine:** roteiro automático que passa por todas as emoções e animações, com opção de movimento ou só rosto.

### Estrutura e build
- Código dividido em arquivos `.h` por função.
- Removido o botão físico. Todo o controle é feito pela página.
- Blocos `delay()` substituídos por controle de tempo não-bloqueante, para o rosto e o Wi-Fi continuarem funcionando.
- Esquema de partição indicado: **Minimal SPIFFS (1.9MB APP with OTA)**. O sketch ocupa cerca de 69%.

## Projeto original

- Robô com duas rodas (servos GPIO 6 e 7), OLED e página de controle por Wi-Fi.
- Botões "hold-to-move" na página, ajuste de Wi-Fi no código-fonte e um modo de movimento aleatório.
- Emoções carregadas do `Emotions.h`, gerado pelo image2cpp.
