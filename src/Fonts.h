// Fontes suaves embutidas (VLW anti-aliased) com cobertura completa do português.
#pragma once
#include <stdint.h>
#include <M5GFX.h>

enum FontId : uint8_t {
  F_TITLE_XL = 0,  // Special Elite 30px — tela de abertura e números grandes
  F_TITLE,         // Special Elite 17px — títulos de casos e telas
  F_BODY,          // IBM Plex Sans Condensed Medium 13px — texto corrido
  F_BODY_L,        // IBM Plex Sans Condensed Medium 16px — texto corrido grande
  F_UI,            // IBM Plex Mono SemiBold 10px — rótulos e dicas de teclas
  F_LABEL,         // IBM Plex Sans Condensed SemiBold 11px — rótulos e listas compactas
  F_COUNT
};

namespace Fonts {

void begin();
const lgfx::IFont* get(FontId id);
void use(M5Canvas& c, FontId id);        // seleciona a fonte no canvas

int lineHeight(FontId id);               // altura natural da linha (ascent + descent)
int advance(FontId id, uint32_t codepoint);
int textWidth(FontId id, const char* s, int byteLen = -1);

// Decodifica um codepoint UTF-8 e avança o ponteiro
uint32_t nextCodepoint(const char*& p, const char* end);

}  // namespace Fonts
