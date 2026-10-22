#include "Fonts.h"
#include "AssetFonts.h"

namespace {

struct FontSlot {
  const uint8_t* data;
  uint32_t size;
  lgfx::PointerWrapper wrapper;
  lgfx::VLWfont font;
  uint8_t adv[256];       // larguras pré-calculadas para 0x00..0xFF
  int lineH;
  bool ok;
};

FontSlot g_slots[F_COUNT] = {
  {FONT_TITLEXL, sizeof(FONT_TITLEXL), {}, {}, {}, 0, false},
  {FONT_TITLE,   sizeof(FONT_TITLE),   {}, {}, {}, 0, false},
  {FONT_BODY,    sizeof(FONT_BODY),    {}, {}, {}, 0, false},
  {FONT_BODYL,   sizeof(FONT_BODYL),   {}, {}, {}, 0, false},
  {FONT_UI,      sizeof(FONT_UI),      {}, {}, {}, 0, false},
  {FONT_LABEL,   sizeof(FONT_LABEL),   {}, {}, {}, 0, false},
};

int slowAdvance(const FontSlot& s, uint32_t cp) {
  if (!s.ok) return 6;
  lgfx::FontMetrics m{};
  if (s.font.updateFontMetric(&m, (uint16_t)cp)) return m.x_advance;
  return s.font.spaceWidth;  // glifo ausente: M5GFX desenha um espaço
}

}  // namespace

namespace Fonts {

void begin() {
  for (int i = 0; i < F_COUNT; ++i) {
    FontSlot& s = g_slots[i];
    s.wrapper.set(s.data, s.size);
    s.ok = s.font.loadFont(&s.wrapper);
    s.lineH = s.ok ? s.font.yAdvance : 10;
    for (int c = 0; c < 256; ++c) {
      int a = slowAdvance(s, (uint32_t)c);
      s.adv[c] = (uint8_t)(a < 0 ? 0 : (a > 255 ? 255 : a));
    }
  }
}

const lgfx::IFont* get(FontId id) {
  const FontSlot& s = g_slots[id < F_COUNT ? id : F_BODY];
  if (!s.ok) return &fonts::Font0;
  return &s.font;
}

void use(M5Canvas& c, FontId id) {
  c.setFont(get(id));
  c.setTextSize(1);
  c.setTextDatum(textdatum_t::top_left);
}

// Entrelinhas ajustadas à mão para a tela de 135 px (a altura natural das fontes
// inclui espaço para acentos em maiúsculas, o que deixaria o texto espaçado demais).
int lineHeight(FontId id) {
  static const int8_t tuned[F_COUNT] = {34, 19, 17, 21, 12, 14};
  if (id >= F_COUNT) id = F_BODY;
  return g_slots[id].ok ? tuned[id] : g_slots[id].lineH;
}

int advance(FontId id, uint32_t cp) {
  const FontSlot& s = g_slots[id < F_COUNT ? id : F_BODY];
  if (cp < 256) return s.adv[cp];
  return slowAdvance(s, cp);
}

uint32_t nextCodepoint(const char*& p, const char* end) {
  const uint8_t c = (uint8_t)*p++;
  if (c < 0x80) return c;
  int extra = 0;
  uint32_t cp = 0;
  if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
  else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
  else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
  else return '?';
  while (extra-- > 0 && p < end) {
    const uint8_t cc = (uint8_t)*p;
    if ((cc & 0xC0) != 0x80) return '?';
    cp = (cp << 6) | (cc & 0x3F);
    ++p;
  }
  return cp;
}

int textWidth(FontId id, const char* s, int byteLen) {
  if (!s) return 0;
  const char* end = s + (byteLen < 0 ? (int)strlen(s) : byteLen);
  int w = 0;
  while (s < end && *s) w += advance(id, nextCodepoint(s, end));
  return w;
}

}  // namespace Fonts
