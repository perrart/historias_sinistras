# Componentes de terceiros

O código e os casos do HISTÓRIAS SINISTRAS estão sob a [PolyForm Noncommercial 1.0.0](LICENSE.md). Os componentes abaixo são de terceiros, entram no firmware ou no projeto e **continuam sob as licenças originais deles**. A licença do jogo não muda nem restringe essas licenças.

## Fontes (embutidas no firmware)

| Fonte | Autor | Licença | Arquivo |
|---|---|---|---|
| Special Elite | Astigmatic (Brian J. Bonislawsky) | Apache License 2.0 | [`assets/fonts/LICENSE-SpecialElite.txt`](assets/fonts/LICENSE-SpecialElite.txt) |
| IBM Plex Sans Condensed e IBM Plex Mono | IBM Corp. | SIL Open Font License 1.1 | [`assets/fonts/OFL-IBMPlex.txt`](assets/fonts/OFL-IBMPlex.txt) |

As fontes foram convertidas para o formato VLW (`src/AssetFonts.h`) somente para uso dentro do jogo.

## Bibliotecas (compiladas no firmware)

| Biblioteca | Autor | Licença |
|---|---|---|
| [M5GFX](https://github.com/m5stack/M5GFX) | M5Stack | MIT |
| [M5Unified](https://github.com/m5stack/M5Unified) | M5Stack | MIT |
| [M5Cardputer](https://github.com/m5stack/M5Cardputer) | M5Stack | ver o repositório oficial |
| [Arduino-ESP32](https://github.com/espressif/arduino-esp32) (núcleo) | Espressif e colaboradores | LGPL 2.1 (partes do núcleo) e outras, ver o repositório |
| [ESP-IDF](https://github.com/espressif/esp-idf) | Espressif | Apache License 2.0 |

O código-fonte completo do jogo está neste repositório. Quem quiser recompilar o firmware com outra versão dessas bibliotecas pode seguir as instruções do README.

Quem redistribuir o firmware (`.bin`) deve levar junto este arquivo e as licenças das fontes listadas acima.
