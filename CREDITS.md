# Créditos e origem

## Projeto original

**Cozmo-MK2 / MK2-Czmo** — repositório original:
https://github.com/nturmose21/MK2-Czmo

- Autor do repositório original: **nturmose21** (dono do repositório no GitHub)
- Commit usado como base desta pasta: `2645bdafe4e867a69b03e713096d14631445437a` ("Add tp4056 module for safe battery charging"), autor do commit `Hack_updt`
- Cópia integral do código e do README original: pasta [`original/`](original/)
- Arquivos de impressão 3D (`3D_Models/`) e molde de papelão (`CARDBOARD TEMPLATE.png`): copiados do repositório original em [`hardware/`](hardware/)
- A imagem do robô exibida no README original pertence ao autor original. Ela não foi copiada para esta pasta.

Esta pasta é uma **derivação**: o código foi reorganizado, reescrito em grande parte e expandido. As mudanças estão em [CHANGELOG.md](CHANGELOG.md).

## Inspirações

- **Anki Cozmo** — o robô que inspirou o projeto original e esta versão (olhos, emoções e comportamentos).
- **Mochan (canal Huy Vector no YouTube)** — a ideia de modos autônomos ("Inquieto" e "Curioso") veio de um projeto de referência. **Nenhum código desse projeto foi copiado.** A implementação em `Autonomy.h` é própria e não usa `delay()`.

## Bibliotecas usadas

| Biblioteca | Autor / mantenedor |
|------------|---------------------|
| ESP32 Arduino core | Espressif Systems |
| ESP32Servo | Kevin Harrington / John K. Bennett |
| Adafruit GFX e Adafruit SSD1306 | Adafruit Industries |
| ESPAsyncWebServer e AsyncTCP (versão ESP32Async) | ESP32Async |

Cada biblioteca mantém a própria licença. Este projeto não redistribui essas bibliotecas.

## Animações

- Animações de GIF criadas por **Augusto** (autor desta versão), convertidas com `Cozmo-MK2/tools/gif2header.py`.
- O bitmap de `shush` (`anim_shush.h`) veio do `Emotions.h` do autor original, gerado com image2cpp.

## Desenvolvimento

Parte do código desta versão foi escrita com auxílio do **Claude** (Anthropic), em conversa com Augusto, que testou, ajustou e carregou o firmware no robô.

---

## Licença

**O repositório original não tem arquivo de licença** (`LICENSE`, `COPYING` ou equivalente).

Na prática, isso significa que o código do autor original está com **todos os direitos reservados**. Consultar o autor é o caminho mais seguro antes de:

- publicar esta pasta (por exemplo, em um repositório público no GitHub);
- redistribuir o código, os arquivos 3D ou o molde.

Para uso pessoal e de estudo, esta pasta serve como referência e não envolve publicação.

**Sugestão:** se for publicar, peça autorização ao autor original (abrindo uma issue no repositório dele) e escolha uma licença para as partes que são suas (por exemplo, MIT). Não coloque uma licença sobre o código original sem a autorização dele.
