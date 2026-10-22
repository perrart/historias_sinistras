// HISTÓRIAS SINISTRAS — configuração global, paleta e constantes de layout.
#pragma once
#include <stdint.h>

#define HS_VERSION "1.2.0"

namespace cfg {

// ---- Tela -----------------------------------------------------------------
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 135;

constexpr int HEADER_H = 14;   // faixa superior
constexpr int FOOTER_H = 13;   // faixa de dicas de controle
constexpr int MARGIN_X = 7;    // margem lateral do texto
constexpr int CONTENT_Y = HEADER_H + 3;
constexpr int CONTENT_H = SCREEN_H - HEADER_H - FOOTER_H - 4;

// ---- Paleta (RGB565) --------------------------------------------------------
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr uint16_t C_BLACK     = rgb(0x09, 0x09, 0x09);  // quase preto
constexpr uint16_t C_CHARCOAL  = rgb(0x1A, 0x1A, 0x1A);  // carvão
constexpr uint16_t C_CHARCOAL2 = rgb(0x24, 0x22, 0x21);  // carvão destacado
constexpr uint16_t C_LINE      = rgb(0x30, 0x2D, 0x2B);  // linhas sutis
constexpr uint16_t C_RED       = rgb(0xB5, 0x12, 0x1B);  // vermelho-sangue
constexpr uint16_t C_RED_DARK  = rgb(0x55, 0x0A, 0x0E);  // vermelho escuro
constexpr uint16_t C_RED_GLOW  = rgb(0xE0, 0x2A, 0x33);  // vermelho aceso
constexpr uint16_t C_PAPER     = rgb(0xD8, 0xCB, 0xB0);  // papel envelhecido
constexpr uint16_t C_PAPER_DIM = rgb(0x9A, 0x90, 0x7C);  // papel apagado
constexpr uint16_t C_WHITE     = rgb(0xF2, 0xF2, 0xF2);  // branco
constexpr uint16_t C_GRAY      = rgb(0x77, 0x77, 0x77);  // cinza
constexpr uint16_t C_GRAY_DARK = rgb(0x48, 0x48, 0x48);  // cinza escuro

// ---- Tempos (ms) -------------------------------------------------------------
constexpr uint32_t REPEAT_DELAY    = 380;   // atraso antes da repetição de setas
constexpr uint32_t REPEAT_RATE     = 75;    // intervalo de repetição
constexpr uint32_t DEBOUNCE_MS     = 25;    // ignora repiques
constexpr uint32_t DIM_AFTER_MS    = 90000; // escurece a tela após inatividade
constexpr uint32_t FRAME_MS        = 16;    // ~60 fps máximo

// ---- Persistência ------------------------------------------------------------
constexpr int MAX_CASE_ID = 255;            // IDs 1..255 cabem no blob de progresso

}  // namespace cfg
