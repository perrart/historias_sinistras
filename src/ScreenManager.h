// Primitivas visuais compartilhadas: cabeçalho, rodapé com teclas, carimbos,
// ícones de status, máscaras de pixel-art e efeitos (glitch, varredura).
#pragma once
#include <stdint.h>
#include <M5GFX.h>
#include "Fonts.h"
#include "ProgressManager.h"

namespace ui {

struct Hint {
  const char* key;    // ex.: "ENTER"
  const char* label;  // ex.: "CONTINUAR"
};

uint16_t blend(uint16_t bg, uint16_t fg, uint8_t alpha);

void clear(M5Canvas& c);
void header(M5Canvas& c, const char* left, const char* right = nullptr, uint16_t rightColor = 0);
void footer(M5Canvas& c, const Hint* hints, int count);
void text(M5Canvas& c, FontId f, int x, int y, const char* s, uint16_t color);
void textCentered(M5Canvas& c, FontId f, int cx, int y, const char* s, uint16_t color);
void textRight(M5Canvas& c, FontId f, int rx, int y, const char* s, uint16_t color);
// Desenha o texto reduzindo a fonte se não couber em maxW. Retorna a fonte usada.
FontId fitText(M5Canvas& c, FontId preferred, FontId fallback, int x, int y, int maxW, const char* s, uint16_t color);
int keycap(M5Canvas& c, int x, int y, const char* key, bool active = true);

void alphaMask(M5Canvas& c, int x, int y, int w, int h, const uint8_t* mask, uint16_t color, uint8_t opacity);
void stamp(M5Canvas& c, int x, int y, const char* label, uint16_t color, uint32_t seed);
void statusIcon(M5Canvas& c, int x, int y, CaseStatus st, uint8_t clues);
void difficultyPips(M5Canvas& c, int x, int y, uint8_t difficulty);
void progressBar(M5Canvas& c, int x, int y, int w, int h, int value, int total);
void battery(M5Canvas& c, int x, int y, int level, bool charging);
void toast(M5Canvas& c, const char* msg);
void vignette(M5Canvas& c, uint16_t color, uint8_t strength);

// Efeitos de pós-processamento aplicados ao framebuffer inteiro
void glitch(M5Canvas& c, uint32_t seed, float intensity);
void wipe(M5Canvas& c, float progress);     // revela de cima para baixo com linha vermelha
void grain(M5Canvas& c, uint32_t seed, int amount);

}  // namespace ui
