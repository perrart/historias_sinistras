#include "StoryRenderer.h"
#include <math.h>
#include <string.h>
#include "Config.h"

namespace {
constexpr int LINE_GAP = 0;        // espaço extra entre linhas
constexpr int PARA_GAP = 4;        // espaço extra entre parágrafos
char g_lineBuf[400];

int lineHeightFor(FontId f) { return Fonts::lineHeight(f) + LINE_GAP; }

// Converte "n codepoints" em bytes a partir de p
int bytesForCps(const char* p, int maxBytes, int cps) {
  const char* s = p;
  const char* end = p + maxBytes;
  while (cps-- > 0 && s < end) Fonts::nextCodepoint(s, end);
  return (int)(s - p);
}
}  // namespace

// ---------------------------------------------------------------------------
// Document
// ---------------------------------------------------------------------------
void Document::clear() {
  _blockCount = 0;
  _lineCount = 0;
  _height = 0;
  _totalCp = 0;
  _laidOut = false;
  _truncated = false;
}

int Document::add(const DocBlock& b) {
  if (_blockCount >= MAX_BLOCKS) return -1;
  _blocks[_blockCount] = b;
  _laidOut = false;
  return _blockCount++;
}

int Document::addText(const char* text, FontId f, uint16_t color, uint8_t spaceBefore, uint8_t indent) {
  DocBlock b;
  b.text = text;
  b.font = f;
  b.color = color;
  b.spaceBefore = spaceBefore;
  b.indent = indent;
  b.kind = BK_TEXT;
  return add(b);
}

int Document::addRule(const char* label, uint16_t lineColor, uint16_t labelColor, uint8_t spaceBefore) {
  DocBlock b;
  b.text = label;
  b.font = F_UI;
  b.color = labelColor;
  b.accent = lineColor;
  b.spaceBefore = spaceBefore;
  b.kind = BK_RULE;
  return add(b);
}

int Document::addSpacer(uint8_t h) {
  DocBlock b;
  b.kind = BK_SPACER;
  b.spaceBefore = h;
  return add(b);
}

int Document::addCenter(const char* text, FontId f, uint16_t color, uint8_t spaceBefore) {
  DocBlock b;
  b.text = text;
  b.font = f;
  b.color = color;
  b.spaceBefore = spaceBefore;
  b.kind = BK_CENTER;
  return add(b);
}

void Document::pushLine(uint16_t block, uint16_t start, uint16_t len, uint8_t cps, int h) {
  if (_lineCount >= MAX_LINES) {
    _truncated = true;
    return;
  }
  Line& l = _lines[_lineCount++];
  l.block = block;
  l.start = start;
  l.len = len;
  l.y = (int16_t)_y;
  l.cpStart = _totalCp;
  l.cpLen = cps;
  l.h = (uint8_t)h;
  _totalCp += cps;
  _y += h;
}

void Document::layout(int width) {
  _width = width;
  _lineCount = 0;
  _totalCp = 0;
  _y = 0;
  _truncated = false;

  for (int bi = 0; bi < _blockCount; ++bi) {
    const DocBlock& b = _blocks[bi];
    if (bi > 0 || b.kind == BK_SPACER) _y += b.spaceBefore;

    if (b.kind == BK_SPACER) continue;

    if (b.kind == BK_RULE) {
      const bool labeled = b.text && *b.text;
      const int h = labeled ? Fonts::lineHeight(F_UI) + 1 : 5;
      pushLine((uint16_t)bi, 0, labeled ? (uint16_t)strlen(b.text) : 0, 1, h);
      continue;
    }

    // BK_TEXT / BK_CENTER: quebra por palavra
    const char* t = b.text ? b.text : "";
    const int n = (int)strlen(t);
    const int lh = lineHeightFor(b.font);
    const int maxW = width - b.indent;
    const int spaceAdv = Fonts::advance(b.font, ' ');

    int pos = 0;
    while (pos <= n) {
      int paraEnd = pos;
      while (paraEnd < n && t[paraEnd] != '\n') ++paraEnd;

      int lineStart = pos;
      int lineW = 0;
      int lineCps = 0;
      int lastBreak = -1;     // byte do último espaço na linha atual
      int wAtBreak = 0;
      int cpsAtBreak = 0;
      const char* p = t + pos;
      const char* end = t + paraEnd;
      bool emittedAny = false;

      while (p < end) {
        const int cpByte = (int)(p - t);
        const uint32_t cp = Fonts::nextCodepoint(p, end);
        const int adv = Fonts::advance(b.font, cp);
        if (cp == ' ') {
          lastBreak = cpByte;
          wAtBreak = lineW;
          cpsAtBreak = lineCps;
        } else if (lineW + adv > maxW && lineCps > 0) {
          if (lastBreak > lineStart) {
            pushLine((uint16_t)bi, (uint16_t)lineStart, (uint16_t)(lastBreak - lineStart), (uint8_t)cpsAtBreak, lh);
            lineW = lineW - wAtBreak - spaceAdv;
            lineCps = lineCps - cpsAtBreak - 1;
            lineStart = lastBreak + 1;
          } else {
            pushLine((uint16_t)bi, (uint16_t)lineStart, (uint16_t)(cpByte - lineStart), (uint8_t)lineCps, lh);
            lineW = 0;
            lineCps = 0;
            lineStart = cpByte;
          }
          lastBreak = -1;
          emittedAny = true;
        }
        lineW += adv;
        ++lineCps;
      }
      // última linha do parágrafo (remove espaços finais)
      int lend = paraEnd;
      while (lend > lineStart && t[lend - 1] == ' ') { --lend; --lineCps; }
      if (lend > lineStart || !emittedAny) {
        pushLine((uint16_t)bi, (uint16_t)lineStart, (uint16_t)(lend - lineStart), (uint8_t)(lineCps < 0 ? 0 : lineCps), lh);
      }
      if (paraEnd >= n) break;
      _y += PARA_GAP;
      pos = paraEnd + 1;
    }
  }
  _height = _y;
  _laidOut = true;
}

uint16_t Document::blockFirstCp(int block) const {
  for (int i = 0; i < _lineCount; ++i)
    if (_lines[i].block >= block) return _lines[i].cpStart;
  return _totalCp;
}

int Document::blockTopY(int block) const {
  for (int i = 0; i < _lineCount; ++i)
    if (_lines[i].block >= block) return _lines[i].y;
  return _height;
}

// ---------------------------------------------------------------------------
// TextView
// ---------------------------------------------------------------------------
void TextView::setViewport(int x, int y, int w, int h) {
  _x = x;
  _y = y;
  _w = w;
  _h = h;
}

void TextView::setDocument(Document* doc) {
  _doc = doc;
  if (_doc && !_doc->laidOut()) _doc->layout(_w);
  _scrollY = 0;
  _targetY = 0;
  _typing = false;
  _reveal = 0xFFFFFFFF;
}

void TextView::startTyping(uint16_t fromCp, uint16_t msPerChar) {
  if (!_doc || msPerChar == 0 || fromCp >= _doc->totalCodepoints()) {
    revealAll();
    return;
  }
  _reveal = fromCp;
  _msPerChar = msPerChar;
  _typing = true;
  _lastTick = 0;
  _accum = 0;
}

void TextView::revealAll() {
  _typing = false;
  _reveal = 0xFFFFFFFF;
}

int TextView::maxScroll() const {
  if (!_doc) return 0;
  const int m = _doc->height() - _h + 2;
  return m > 0 ? m : 0;
}

bool TextView::canScroll() const { return maxScroll() > 0; }
bool TextView::atBottom() const { return _targetY >= maxScroll(); }

void TextView::scrollTo(int y, bool animate) {
  if (y < 0) y = 0;
  if (y > maxScroll()) y = maxScroll();
  _targetY = y;
  if (!animate) _scrollY = (float)y;
}

void TextView::scrollToBlock(int block, bool animate) {
  if (!_doc) return;
  scrollTo(_doc->blockTopY(block) - 2, animate);
}

bool TextView::update(uint32_t now) {
  bool dirty = false;
  _typedTick = false;
  // rolagem suave
  const float diff = (float)_targetY - _scrollY;
  if (fabsf(diff) > 0.5f) {
    float step = diff * 0.35f;
    if (fabsf(step) < 1.0f) step = diff > 0 ? 1.0f : -1.0f;
    _scrollY += step;
    dirty = true;
  } else if (_scrollY != (float)_targetY) {
    _scrollY = (float)_targetY;
    dirty = true;
  }
  // máquina de escrever
  if (_typing && _doc) {
    if (_lastTick == 0) _lastTick = now;
    _accum += (float)(now - _lastTick);
    _lastTick = now;
    const uint32_t before = _reveal;
    while (_accum >= _msPerChar) {
      _accum -= _msPerChar;
      ++_reveal;
    }
    // o efeito só anima o que está visível na janela; o restante aparece de uma vez
    uint32_t visibleEnd = _doc->totalCodepoints();
    const int bottom = (int)_scrollY + _h;
    for (int i = 0; i < _doc->lineCount(); ++i) {
      if (_doc->line(i).y >= bottom) {
        visibleEnd = _doc->line(i).cpStart;
        break;
      }
    }
    if (_reveal >= visibleEnd) revealAll();
    if (_reveal != before) {
      dirty = true;
      _typedTick = true;
    }
  }
  return dirty;
}

bool TextView::handleInput(const InputEvent& ev) {
  if (!_doc) return false;
  const int step = Fonts::lineHeight(F_BODY) + 3;
  const int page = _h - step;
  switch (ev.key) {
    case K_UP:
      if (_targetY <= 0) return false;
      scrollTo(_targetY - step);
      return true;
    case K_DOWN:
      if (_targetY >= maxScroll()) return false;
      scrollTo(_targetY + step);
      return true;
    case K_LEFT:
      if (_targetY <= 0) return false;
      scrollTo(_targetY - page);
      return true;
    case K_RIGHT:
    case K_SPACE:
      if (_targetY >= maxScroll()) return false;
      scrollTo(_targetY + page);
      return true;
    default:
      return false;
  }
}

void TextView::draw(M5Canvas& c) {
  if (!_doc) return;
  const int sy = (int)lroundf(_scrollY);
  c.setClipRect(_x, _y, _w + 1, _h);
  for (int i = 0; i < _doc->lineCount(); ++i) {
    const Document::Line& ln = _doc->line(i);
    const int ly = _y + ln.y - sy;
    if (ly + ln.h < _y) continue;
    if (ly > _y + _h) break;
    if (ln.cpStart >= _reveal) break;
    const DocBlock& b = _doc->block(ln.block);

    if (b.kind == BK_RULE) {
      const int mid = (b.text && *b.text) ? ly + 6 : ly + 2;
      int lx = _x;
      if (b.text && *b.text) {
        Fonts::use(c, F_UI);
        c.setTextColor(b.color);
        c.drawString(b.text, _x, ly - 2);
        lx = _x + Fonts::textWidth(F_UI, b.text) + 5;
      }
      if (lx < _x + _w) c.drawFastHLine(lx, mid, _x + _w - lx, b.accent);
      continue;
    }

    int cps = ln.cpLen;
    if (_reveal != 0xFFFFFFFF && (uint32_t)(ln.cpStart + cps) > _reveal) cps = (int)(_reveal - ln.cpStart);
    const char* src = b.text + ln.start;
    int bytes = (cps == ln.cpLen) ? ln.len : bytesForCps(src, ln.len, cps);
    if (bytes > (int)sizeof(g_lineBuf) - 1) bytes = sizeof(g_lineBuf) - 1;
    memcpy(g_lineBuf, src, bytes);
    g_lineBuf[bytes] = 0;

    int lx = _x + b.indent;
    if (b.kind == BK_CENTER) {
      const int full = Fonts::textWidth(b.font, src, ln.len);
      lx = _x + (_w - full) / 2;
    }
    Fonts::use(c, b.font);
    c.setTextColor(b.color);
    c.drawString(g_lineBuf, lx, ly);
    // cursor de máquina de escrever
    if (_typing && cps < ln.cpLen) {
      const int cx = lx + Fonts::textWidth(b.font, g_lineBuf, bytes) + 1;
      c.fillRect(cx, ly + 3, 2, Fonts::lineHeight(b.font) - 5, cfg::C_RED);
    }
  }
  c.clearClipRect();

  // barra de rolagem
  const int ms = maxScroll();
  if (ms > 0) {
    const int trackX = _x + _w + 3;
    c.drawFastVLine(trackX, _y, _h, cfg::C_LINE);
    int thumbH = _h * _h / (_doc->height() + 1);
    if (thumbH < 8) thumbH = 8;
    const int thumbY = _y + (int)((long)(_h - thumbH) * sy / ms);
    c.fillRect(trackX - 1, thumbY, 3, thumbH, cfg::C_RED);
  }
}
