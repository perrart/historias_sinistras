// Telas do jogo. Cada uma implementa a interface Screen (GameEngine.h).
#pragma once
#include "GameEngine.h"

namespace screens {
Screen* get(ScreenId id);

// Instâncias definidas em cada arquivo Screens*.cpp
Screen* splash();
Screen* menu();
Screen* howTo();
Screen* credits();
Screen* intro();
Screen* hub();
Screen* clues();
Screen* truth();
Screen* browser();
Screen* resolved();
Screen* settings();
Screen* reset();
}  // namespace screens

// Teclas especiais desenhadas como setas no rodapé
#define KEYCAP_UPDOWN "\x01"
#define KEYCAP_LEFTRIGHT "\x02"
