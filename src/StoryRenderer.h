// Renderização de textos longos: quebra de linha por palavra (UTF-8), blocos com
// estilos diferentes, rolagem suave e efeito de máquina de escrever.
// Não usa String nem alocação dinâmica: as linhas são índices para o texto original.
#pragma once
#include <stdint.h>
#include <M5GFX.h>
#include "Fonts.h"
#include "InputManager.h"

enum BlockKind : uint8_t {
  BK_TEXT = 0,   // parágrafos com quebra automática
  BK_RULE,       // linha horizontal (com rótulo opcional, ex.: "DICA 02 / 04")
  BK_SPACER,     // espaço vertical
  BK_CENTER,     // texto centralizado (sem quebra automática se couber)
};

struct DocBlock {
  const char* text = nullptr;
  FontId font = F_BODY;
  uint16_t color = 0xFFFF;
  uint8_t spaceBefore = 0;
  uint8_t kind = BK_TEXT;
  uint8_t indent = 0;
  uint16_t accent = 0;     // cor da linha em BK_RULE
};

class Document {
 public:
  static constexpr int MAX_BLOCKS = 24;
  static constexpr int MAX_LINES = 220;

  struct Line {
    uint16_t block;
    uint16_t start;     // byte inicial no texto do bloco
    uint16_t len;       // bytes
    int16_t y;          // topo da linha no documento
    uint16_t cpStart;   // índice global do primeiro codepoint (para a máquina de escrever)
    uint8_t cpLen;
    uint8_t h;
  };

  void clear();
  int add(const DocBlock& b);            // retorna índice do bloco
  int addText(const char* text, FontId f, uint16_t color, uint8_t spaceBefore = 0, uint8_t indent = 0);
  int addRule(const char* label, uint16_t lineColor, uint16_t labelColor, uint8_t spaceBefore = 4);
  int addSpacer(uint8_t h);
  int addCenter(const char* text, FontId f, uint16_t color, uint8_t spaceBefore = 0);

  void layout(int width);
  bool laidOut() const { return _laidOut; }

  int blockCount() const { return _blockCount; }
  const DocBlock& block(int i) const { return _blocks[i]; }
  int lineCount() const { return _lineCount; }
  const Line& line(int i) const { return _lines[i]; }
  int height() const { return _height; }
  int width() const { return _width; }
  uint16_t totalCodepoints() const { return _totalCp; }
  uint16_t blockFirstCp(int block) const;
  int blockTopY(int block) const;
  bool truncated() const { return _truncated; }

 private:
  void pushLine(uint16_t block, uint16_t start, uint16_t len, uint8_t cps, int h);
  DocBlock _blocks[MAX_BLOCKS];
  Line _lines[MAX_LINES];
  int _blockCount = 0;
  int _lineCount = 0;
  int _height = 0;
  int _width = 0;
  int _y = 0;
  uint16_t _totalCp = 0;
  bool _laidOut = false;
  bool _truncated = false;
};

// Janela rolável que exibe um Document.
class TextView {
 public:
  void setViewport(int x, int y, int w, int h);
  void setDocument(Document* doc);
  // Começa o efeito de digitação a partir do codepoint indicado (anteriores já visíveis).
  // msPerChar = 0 revela tudo imediatamente.
  void startTyping(uint16_t fromCp, uint16_t msPerChar);
  void revealAll();
  bool typing() const { return _typing; }

  bool update(uint32_t now);              // true se precisa redesenhar
  bool handleInput(const InputEvent& ev); // rolagem; true se consumiu
  void draw(M5Canvas& c);

  void scrollTo(int y, bool animate = true);
  void scrollToBlock(int block, bool animate = true);
  bool canScroll() const;
  bool atTop() const { return _targetY <= 0; }
  bool atBottom() const;
  int x() const { return _x; }
  int y() const { return _y; }
  int w() const { return _w; }
  int h() const { return _h; }
  bool typedSomething() const { return _typedTick; }
  int scrollY() const { return (int)(_scrollY + 0.5f); }
  Document* document() const { return _doc; }

 private:
  int maxScroll() const;
  Document* _doc = nullptr;
  int _x = 0, _y = 0, _w = 0, _h = 0;
  float _scrollY = 0;
  int _targetY = 0;
  bool _typing = false;
  uint32_t _reveal = 0xFFFFFFFF;  // codepoints visíveis
  uint16_t _msPerChar = 0;
  uint32_t _lastTick = 0;
  float _accum = 0;
  bool _typedTick = false;
};
