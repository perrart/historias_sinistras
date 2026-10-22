#include "ScreenManager.h"
#include <string.h>
#include "Config.h"

using namespace cfg;

namespace {

inline bool direct(M5Canvas& c) { return c.getBuffer() && c.getColorDepth() == 16; }
inline uint16_t* pix(M5Canvas& c) { return (uint16_t*)c.getBuffer(); }
inline uint16_t swap16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }

struct Rng {
  uint32_t s;
  explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  int range(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }
};

void drawArrowUp(M5Canvas& c, int x, int y, uint16_t col) { c.fillTriangle(x, y + 4, x + 4, y + 4, x + 2, y, col); }
void drawArrowDown(M5Canvas& c, int x, int y, uint16_t col) { c.fillTriangle(x, y, x + 4, y, x + 2, y + 4, col); }
void drawArrowLeft(M5Canvas& c, int x, int y, uint16_t col) { c.fillTriangle(x + 4, y, x + 4, y + 4, x, y + 2, col); }
void drawArrowRight(M5Canvas& c, int x, int y, uint16_t col) { c.fillTriangle(x, y, x, y + 4, x + 4, y + 2, col); }

}  // namespace

namespace ui {

uint16_t blend(uint16_t bg, uint16_t fg, uint8_t a) {
  const uint32_t r1 = (bg >> 11) & 0x1F, g1 = (bg >> 5) & 0x3F, b1 = bg & 0x1F;
  const uint32_t r2 = (fg >> 11) & 0x1F, g2 = (fg >> 5) & 0x3F, b2 = fg & 0x1F;
  const uint32_t r = (r1 * (255 - a) + r2 * a) / 255;
  const uint32_t g = (g1 * (255 - a) + g2 * a) / 255;
  const uint32_t b = (b1 * (255 - a) + b2 * a) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

void clear(M5Canvas& c) { c.fillScreen(C_BLACK); }

void text(M5Canvas& c, FontId f, int x, int y, const char* s, uint16_t color) {
  Fonts::use(c, f);
  c.setTextColor(color);
  c.drawString(s, x, y);
}

void textCentered(M5Canvas& c, FontId f, int cx, int y, const char* s, uint16_t color) {
  text(c, f, cx - Fonts::textWidth(f, s) / 2, y, s, color);
}

void textRight(M5Canvas& c, FontId f, int rx, int y, const char* s, uint16_t color) {
  text(c, f, rx - Fonts::textWidth(f, s), y, s, color);
}

FontId fitText(M5Canvas& c, FontId preferred, FontId fallback, int x, int y, int maxW, const char* s,
               uint16_t color) {
  FontId f = Fonts::textWidth(preferred, s) <= maxW ? preferred : fallback;
  text(c, f, x, y + (f == preferred ? 0 : (Fonts::lineHeight(preferred) - Fonts::lineHeight(f)) / 2), s, color);
  return f;
}

void header(M5Canvas& c, const char* left, const char* right, uint16_t rightColor) {
  c.fillRect(0, 0, SCREEN_W, HEADER_H - 1, C_CHARCOAL);
  c.drawFastHLine(0, HEADER_H - 1, SCREEN_W, C_LINE);
  c.drawFastHLine(0, HEADER_H - 1, 38, C_RED);
  c.fillRect(5, 4, 4, 5, C_RED);
  if (left) text(c, F_UI, 13, -1, left, C_PAPER);
  if (right) textRight(c, F_UI, SCREEN_W - 5, -1, right, rightColor ? rightColor : C_GRAY);
}

int keycap(M5Canvas& c, int x, int y, const char* key, bool active) {
  const uint16_t edge = active ? C_PAPER_DIM : C_GRAY_DARK;
  const uint16_t fg = active ? C_PAPER : C_GRAY;
  int w;
  if (key[0] == '\x01' || key[0] == '\x02') {  // setas desenhadas
    w = 15;
    c.drawRoundRect(x, y, w, 10, 2, edge);
    if (key[0] == '\x01') {
      drawArrowUp(c, x + 3, y + 2, fg);
      drawArrowDown(c, x + 8, y + 3, fg);
    } else {
      drawArrowLeft(c, x + 3, y + 3, fg);
      drawArrowRight(c, x + 8, y + 3, fg);
    }
    return w;
  }
  const int tw = Fonts::textWidth(F_UI, key);
  w = tw + 6;
  c.fillRoundRect(x, y, w, 10, 2, C_CHARCOAL2);
  c.drawRoundRect(x, y, w, 10, 2, edge);
  text(c, F_UI, x + 3, y - 3, key, fg);
  return w;
}

void footer(M5Canvas& c, const Hint* hints, int count) {
  const int y0 = SCREEN_H - FOOTER_H;
  c.fillRect(0, y0, SCREEN_W, FOOTER_H, C_CHARCOAL);
  c.drawFastHLine(0, y0, SCREEN_W, C_LINE);
  // mede o total para decidir o espaçamento
  int total = 0;
  for (int i = 0; i < count; ++i) {
    const char* k = hints[i].key;
    const int kw = (k[0] == '\x01' || k[0] == '\x02') ? 15 : Fonts::textWidth(F_UI, k) + 6;
    total += kw + 3 + Fonts::textWidth(F_UI, hints[i].label);
  }
  const bool compact = total + (count - 1) * 3 > SCREEN_W - 8;  // não cabe: aperta
  int gap = count > 1 ? (SCREEN_W - 8 - total) / (count - 1) : 0;
  if (gap > 12) gap = 12;
  if (gap < 3) gap = 3;
  int x = 4;
  for (int i = 0; i < count; ++i) {
    x += keycap(c, x, y0 + 2, hints[i].key);
    x += 3;
    text(c, F_UI, x, y0 - 1, hints[i].label, C_GRAY);
    x += Fonts::textWidth(F_UI, hints[i].label) + (compact ? 2 : gap);
  }
}

void alphaMask(M5Canvas& c, int x, int y, int w, int h, const uint8_t* mask, uint16_t color, uint8_t opacity) {
  if (!direct(c)) return;
  uint16_t* b = pix(c);
  const int W = c.width(), H = c.height();
  for (int j = 0; j < h; ++j) {
    const int yy = y + j;
    if (yy < 0 || yy >= H) continue;
    for (int i = 0; i < w; ++i) {
      const int xx = x + i;
      if (xx < 0 || xx >= W) continue;
      const uint8_t m = mask[j * w + i];
      if (!m) continue;
      const uint8_t a = (uint8_t)((m * opacity) / 255);
      uint16_t& p = b[yy * W + xx];
      p = swap16(blend(swap16(p), color, a));
    }
  }
}

void stamp(M5Canvas& c, int x, int y, const char* label, uint16_t color, uint32_t seed) {
  const int tw = Fonts::textWidth(F_TITLE, label);
  const int w = tw + 12, h = Fonts::lineHeight(F_TITLE) + 6;
  c.drawRect(x, y, w, h, color);
  c.drawRect(x + 1, y + 1, w - 2, h - 2, color);
  text(c, F_TITLE, x + 6, y + 3, label, color);
  if (!direct(c)) return;
  // falhas de tinta
  Rng r(seed);
  uint16_t* b = pix(c);
  const int W = c.width();
  for (int i = 0; i < w * h / 7; ++i) {
    const int px = x + r.range(0, w - 1), py = y + r.range(0, h - 1);
    if (px < 0 || py < 0 || px >= W || py >= c.height()) continue;
    uint16_t& p = b[py * W + px];
    p = swap16(blend(swap16(p), C_BLACK, (uint8_t)r.range(90, 200)));
  }
}

void statusIcon(M5Canvas& c, int x, int y, CaseStatus st, uint8_t clues) {
  switch (st) {
    case STATUS_NEW:
      c.drawCircle(x + 6, y + 4, 3, C_GRAY_DARK);
      break;
    case STATUS_IN_PROGRESS:
      for (int i = 0; i < 4; ++i) {
        if (i < clues) c.fillRect(x + i * 3, y, 2, 9, C_PAPER);
        else c.drawRect(x + i * 3, y, 2, 9, C_GRAY_DARK);
      }
      break;
    case STATUS_RESOLVED:
      c.fillRect(x + 1, y, 10, 9, C_RED);
      c.drawLine(x + 3, y + 4, x + 5, y + 6, C_PAPER);
      c.drawLine(x + 5, y + 6, x + 9, y + 2, C_PAPER);
      c.drawLine(x + 3, y + 5, x + 5, y + 7, C_PAPER);
      c.drawLine(x + 5, y + 7, x + 9, y + 3, C_PAPER);
      break;
  }
}

void difficultyPips(M5Canvas& c, int x, int y, uint8_t difficulty) {
  for (int i = 0; i < 4; ++i) {
    if (i <= difficulty) c.fillRect(x + i * 6, y, 4, 4, difficulty == 3 ? C_RED_GLOW : C_RED);
    else c.drawRect(x + i * 6, y, 4, 4, C_GRAY_DARK);
  }
}

void progressBar(M5Canvas& c, int x, int y, int w, int h, int value, int total) {
  c.fillRect(x, y, w, h, C_CHARCOAL2);
  c.drawRect(x, y, w, h, C_LINE);
  if (total > 0 && value > 0) {
    int fw = (w - 2) * value / total;
    if (fw < 1) fw = 1;
    c.fillRect(x + 1, y + 1, fw, h - 2, C_RED);
  }
  for (int i = 1; i < 10; ++i) c.drawFastVLine(x + w * i / 10, y + h - 2, 1, C_BLACK);
}

void battery(M5Canvas& c, int x, int y, int level, bool charging) {
  if (level < 0) return;
  c.drawRect(x, y, 14, 7, C_GRAY);
  c.fillRect(x + 14, y + 2, 2, 3, C_GRAY);
  const int fw = 12 * level / 100;
  c.fillRect(x + 1, y + 1, fw < 1 ? 1 : fw, 5, level < 20 ? C_RED : C_PAPER_DIM);
  if (charging) {
    c.drawLine(x + 7, y + 1, x + 5, y + 4, C_WHITE);
    c.drawLine(x + 5, y + 4, x + 8, y + 3, C_WHITE);
    c.drawLine(x + 8, y + 3, x + 6, y + 6, C_WHITE);
  }
}

void toast(M5Canvas& c, const char* msg) {
  const int tw = Fonts::textWidth(F_LABEL, msg);
  const int w = tw + 16, h = 18;
  const int x = (SCREEN_W - w) / 2, y = SCREEN_H - FOOTER_H - h - 6;
  c.fillRect(x, y, w, h, C_CHARCOAL2);
  c.drawRect(x, y, w, h, C_RED);
  c.fillRect(x, y, 3, h, C_RED);
  text(c, F_LABEL, x + 9, y + 2, msg, C_WHITE);
}

void vignette(M5Canvas& c, uint16_t color, uint8_t strength) {
  const int rings = 12;
  for (int i = 0; i < rings; ++i) {
    const uint8_t a = (uint8_t)(strength * (rings - i) / rings / 3);
    if (!a) continue;
    c.fillRectAlpha(i, i, SCREEN_W - 2 * i, 1, a, color);
    c.fillRectAlpha(i, SCREEN_H - 1 - i, SCREEN_W - 2 * i, 1, a, color);
    c.fillRectAlpha(i, i + 1, 1, SCREEN_H - 2 * i - 2, a, color);
    c.fillRectAlpha(SCREEN_W - 1 - i, i + 1, 1, SCREEN_H - 2 * i - 2, a, color);
  }
}

void glitch(M5Canvas& c, uint32_t seed, float intensity) {
  if (!direct(c) || intensity <= 0) return;
  Rng r(seed);
  uint16_t* b = pix(c);
  const int W = c.width(), H = c.height();
  static uint16_t row[SCREEN_W];
  const int slices = 2 + (int)(intensity * 9);
  for (int s = 0; s < slices; ++s) {
    const int y = r.range(0, H - 1);
    const int h = r.range(1, 2 + (int)(intensity * 6));
    int dx = r.range(1, 2 + (int)(intensity * 18));
    if (r.next() & 1) dx = -dx;
    const bool tint = (r.next() % 3) == 0;
    for (int yy = y; yy < y + h && yy < H; ++yy) {
      uint16_t* line = b + yy * W;
      for (int x = 0; x < W; ++x) {
        int sx = x - dx;
        if (sx < 0) sx += W;
        if (sx >= W) sx -= W;
        row[x] = line[sx];
      }
      if (tint) {
        for (int x = 0; x < W; ++x) row[x] = swap16(blend(swap16(row[x]), C_RED, 90));
      }
      memcpy(line, row, W * sizeof(uint16_t));
    }
  }
  // riscos claros
  const int streaks = (int)(intensity * 4);
  for (int s = 0; s < streaks; ++s) {
    const int y = r.range(0, H - 1);
    const int x = r.range(0, W - 40);
    const int len = r.range(10, 60);
    for (int i = 0; i < len && x + i < W; ++i) {
      uint16_t& p = b[y * W + x + i];
      p = swap16(blend(swap16(p), C_PAPER, 110));
    }
  }
}

void wipe(M5Canvas& c, float progress) {
  if (progress >= 1.0f) return;
  int y = (int)(progress * SCREEN_H);
  if (y < 0) y = 0;
  if (y + 2 < SCREEN_H) c.fillRect(0, y + 2, SCREEN_W, SCREEN_H - y - 2, C_BLACK);
  c.fillRect(0, y, SCREEN_W, 2, C_RED_GLOW);
  for (int i = 1; i <= 4; ++i) {
    if (y - i < 0) break;
    c.fillRectAlpha(0, y - i, SCREEN_W, 1, (uint8_t)(120 - i * 25), C_RED);
  }
}

void grain(M5Canvas& c, uint32_t seed, int amount) {
  if (!direct(c)) return;
  Rng r(seed);
  uint16_t* b = pix(c);
  const int W = c.width(), H = c.height();
  for (int i = 0; i < amount; ++i) {
    uint16_t& p = b[r.range(0, H - 1) * W + r.range(0, W - 1)];
    p = swap16(blend(swap16(p), (r.next() & 1) ? C_GRAY_DARK : C_BLACK, 150));
  }
}

}  // namespace ui
