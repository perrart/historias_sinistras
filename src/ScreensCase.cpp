// Fluxo de um caso: apresentação → investigação → dicas → a verdade.
// O caso só é marcado como resolvido quando o jogador escolhe isso no menu.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "AssetArt.h"
#include "Config.h"
#include "Platform.h"
#include "ScreenManager.h"
#include "Screens.h"
#include "StoryRenderer.h"

using namespace cfg;

namespace {

char g_caseLabel[24];
char g_solvedToast[48];

const char* caseLabel() {
  snprintf(g_caseLabel, sizeof(g_caseLabel), "CASO #%03u", (unsigned)Game.caseId());
  return g_caseLabel;
}

void setupView(TextView& v, Document& d) {
  v.setViewport(MARGIN_X, CONTENT_Y, SCREEN_W - 2 * MARGIN_X - 4, CONTENT_H);
  d.layout(v.w());
  v.setDocument(&d);
}

void typeTick(TextView& v) {
  if (v.typedSomething() && Game.settings.data().textSpeed == SPEED_NORMAL) Game.audio.play(SFX_TYPE);
}

// Marca/desmarca o caso atual como resolvido. Retorna o novo estado.
bool toggleResolved() {
  const uint16_t id = Game.caseId();
  const bool on = !Game.progress.resolved(id);
  Game.progress.setResolved(id, on);
  Game.progress.save();
  if (on) {
    Game.audio.play(SFX_SOLVED);
    snprintf(g_solvedToast, sizeof(g_solvedToast), "CASO ENCERRADO · %d/%d RESOLVIDOS", Game.resolvedCount(),
             MysteryDB::count());
    Game.toast(g_solvedToast, 2400);
  } else {
    Game.audio.play(SFX_BACK);
    Game.toast("CASO REABERTO.");
  }
  Game.redraw();
  return on;
}

void goBackFromCase() {
  Game.audio.play(SFX_BACK);
  Game.go(Game.caseReturn, TR_GLITCH);
}

// ===========================================================================
// APRESENTAÇÃO DO CASO (passo 1)
// ===========================================================================
class IntroScreen : public Screen {
 public:
  void enter() override {
    const Mystery& m = Game.mystery();
    _reread = Game.introReread;
    _doc.clear();
    _doc.addText(MysteryDB::categoryName(m.category), F_UI, C_RED, 0);
    _doc.addText(m.title, F_TITLE, C_WHITE, 1);
    _doc.addRule(nullptr, C_LINE, C_GRAY, 3);
    const int scen = _doc.addText(m.scenario, Game.bodyFont(), C_PAPER, 3);
    if (!_reread) _doc.addCenter("VOCÊ CONSEGUE DESCOBRIR O QUE REALMENTE ACONTECEU?", F_UI, C_RED, 9);
    setupView(_view, _doc);
    if (!_reread) _view.startTyping(_doc.blockFirstCp(scen), Game.typeMsPerChar());
    Game.progress.markOpened(m.id);
  }

  void input(const InputEvent& ev) override {
    if (_view.typing()) {
      _view.revealAll();
      Game.redraw();
      return;
    }
    if (_view.handleInput(ev)) {
      Game.redraw();
      return;
    }
    if (ev.key == K_ENTER) {
      Game.audio.play(SFX_SELECT);
      Game.go(SCR_HUB, TR_GLITCH);
    } else if (ev.key == K_BACK) {
      if (_reread) {
        Game.audio.play(SFX_BACK);
        Game.go(SCR_HUB, TR_GLITCH);
      } else {
        goBackFromCase();
      }
    } else if (ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    } else if (ev.ch == '1' || ev.ch == '2') {
      // atalhos diretos para a investigação
      Game.audio.play(SFX_SELECT);
      Game.go(SCR_HUB, TR_GLITCH);
    }
  }

  bool update(uint32_t now) override {
    const bool d = _view.update(now);
    typeTick(_view);
    return d;
  }

  void draw(M5Canvas& c) override {
    const Mystery& m = Game.mystery();
    ui::clear(c);
    ui::header(c, caseLabel(), MysteryDB::difficultyName(m.difficulty), m.difficulty >= DIFF_DIFICIL ? C_RED : C_GRAY);
    const int dw = Fonts::textWidth(F_UI, MysteryDB::difficultyName(m.difficulty));
    ui::difficultyPips(c, SCREEN_W - 5 - dw - 28, 5, m.difficulty);
    _view.draw(c);
    if (_reread) {
      static const ui::Hint h[] = {{KEYCAP_UPDOWN, "ROLAR"}, {"ENTER", "INVESTIGAR"}, {"ESC", "VOLTAR"}};
      ui::footer(c, h, 3);
    } else {
      static const ui::Hint h[] = {{"ENTER", "INVESTIGAR"}, {"ESC", "VOLTAR"}, {KEYCAP_UPDOWN, "ROLAR"}};
      ui::footer(c, h, _view.canScroll() ? 3 : 2);
    }
  }

 private:
  Document _doc;
  TextView _view;
  bool _reread = false;
};

// ===========================================================================
// INVESTIGAÇÃO (passo 2)
// ===========================================================================
class HubScreen : public Screen {
 public:
  void enter() override {
    const uint8_t n = Game.progress.clues(Game.caseId());
    _sel = n < 4 ? 0 : 1;
    _stampAt = 0;
  }

  void input(const InputEvent& ev) override {
    if (ev.key == K_UP) {
      _sel = (_sel + ITEMS - 1) % ITEMS;
      Game.audio.play(SFX_NAV);
      Game.redraw();
    } else if (ev.key == K_DOWN) {
      _sel = (_sel + 1) % ITEMS;
      Game.audio.play(SFX_NAV);
      Game.redraw();
    } else if (ev.key == K_ENTER) {
      activate(_sel);
    } else if (ev.ch >= '1' && ev.ch <= '5') {
      _sel = ev.ch - '1';
      activate(_sel);
    } else if (ev.ch == 'r') {
      activate(2);
    } else if (ev.ch == 'd') {
      activate(3);
    } else if (ev.ch == 's') {
      _sel = 4;
      activate(4);
    } else if (ev.ch == 'n') {
      Game.openCase(Game.nextCaseIndex(Game.caseIndex));
    } else if (ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    } else if (ev.key == K_BACK) {
      goBackFromCase();
    }
  }

  bool update(uint32_t now) override {
    if (!_stampAt) return false;
    if (now - _stampAt > 1300) {
      _stampAt = 0;
      return true;
    }
    return true;
  }

  void draw(M5Canvas& c) override {
    const Mystery& m = Game.mystery();
    const uint16_t id = m.id;
    const uint8_t n = Game.progress.clues(id);
    const bool solved = Game.progress.resolved(id);

    ui::clear(c);
    ui::header(c, caseLabel(), solved ? "RESOLVIDO" : "INVESTIGAÇÃO", solved ? C_RED : C_GRAY);
    ui::fitText(c, F_TITLE, F_LABEL, MARGIN_X, 17, SCREEN_W - 2 * MARGIN_X, m.title, C_WHITE);

    // evidências
    const int ey = 36;
    ui::text(c, F_UI, MARGIN_X, ey, "DICAS", C_GRAY);
    for (int i = 0; i < 4; ++i) {
      const int x = 44 + i * 22;
      char lbl[4];
      snprintf(lbl, sizeof(lbl), "%02d", i + 1);
      if (i < n) {
        c.fillRect(x, ey + 1, 19, 11, C_RED);
        ui::text(c, F_UI, x + 4, ey, lbl, C_PAPER);
      } else {
        c.drawRect(x, ey + 1, 19, 11, C_GRAY_DARK);
        ui::text(c, F_UI, x + 4, ey, lbl, C_GRAY_DARK);
      }
    }
    char avail[24];
    snprintf(avail, sizeof(avail), "DISPONÍVEIS: %d", 4 - n);
    ui::textRight(c, F_UI, SCREEN_W - MARGIN_X, ey, avail, 4 - n > 0 ? C_PAPER_DIM : C_GRAY_DARK);

    // ações
    char l1[40], l4[40];
    const char* l5 = solved ? "DESMARCAR COMO RESOLVIDO" : "MARCAR COMO RESOLVIDO";
    if (n < 4) snprintf(l1, sizeof(l1), "REVELAR DICA %02d", n + 1);
    else snprintf(l1, sizeof(l1), "TODAS AS DICAS REVELADAS");
    if (n > 0) snprintf(l4, sizeof(l4), "VER DICAS REVELADAS (%d)", n);
    else snprintf(l4, sizeof(l4), "NENHUMA DICA REVELADA");
    const char* labels[ITEMS] = {l1, solved ? "VER A VERDADE DE NOVO" : "REVELAR A VERDADE", "RELER O CASO", l4, l5};
    const bool enabled[ITEMS] = {n < 4, true, true, n > 0, true};
    for (int i = 0; i < ITEMS; ++i) {
      const int y = 50 + i * 14;
      const bool on = i == _sel;
      if (on) {
        c.fillRect(MARGIN_X - 2, y - 1, SCREEN_W - 2 * MARGIN_X + 4, 13, C_CHARCOAL2);
        c.fillRect(MARGIN_X - 2, y - 1, 3, 13, C_RED);
      }
      char k[2] = {(char)('1' + i), 0};
      ui::keycap(c, MARGIN_X + 5, y + 1, k, enabled[i]);
      uint16_t col = !enabled[i] ? C_GRAY_DARK : (on ? C_WHITE : C_PAPER_DIM);
      if (i == 1 && enabled[i] && !solved) col = on ? C_RED_GLOW : C_RED;
      ui::text(c, F_LABEL, MARGIN_X + 22, y - 1, labels[i], col);
      if (i == 4 && solved) {  // selo de resolvido ao lado do item
        const int x = MARGIN_X + 22 + Fonts::textWidth(F_LABEL, labels[i]) + 6;
        c.drawFastHLine(x, y + 5, 4, C_RED);
        c.drawLine(x + 3, y + 8, x + 9, y + 1, C_RED);
        c.drawLine(x + 3, y + 7, x + 9, y + 0, C_RED);
      }
    }

    // carimbo ao marcar como resolvido
    if (_stampAt) {
      const uint32_t st = hal::now() - _stampAt;
      const int jitter = st < 160 ? (int)((160 - st) / 40) : 0;
      const int sw = Fonts::textWidth(F_TITLE, "RESOLVIDO") + 12, sh = Fonts::lineHeight(F_TITLE) + 6;
      const int sx = SCREEN_W - MARGIN_X - sw + jitter, sy = 18 - jitter;
      c.fillRect(sx - 2, sy - 2, sw + 4, sh + 4, C_BLACK);
      ui::stamp(c, sx, sy, "RESOLVIDO", C_RED, id * 977u);
    }

    static const ui::Hint h[] = {{"1", "DICA"}, {"2", "VERDADE"}, {"ENTER", "OK"}, {"ESC", "SAIR"}};
    ui::footer(c, h, 4);
  }

 private:
  void activate(int which) {
    const uint16_t id = Game.caseId();
    const uint8_t n = Game.progress.clues(id);
    switch (which) {
      case 0:
        if (n >= 4) {
          Game.audio.play(SFX_DENY);
          Game.toast("NÃO HÁ MAIS DICAS. CONFIE NO SEU INSTINTO.");
          return;
        }
        Game.progress.setClues(id, n + 1);
        Game.clueJustRevealed = true;
        Game.go(SCR_CLUES, TR_NONE);
        break;
      case 1:
        Game.audio.play(SFX_SELECT);
        Game.go(SCR_TRUTH, TR_FLASH);
        break;
      case 2:
        Game.audio.play(SFX_SELECT);
        Game.introReread = true;
        Game.go(SCR_INTRO, TR_GLITCH);
        break;
      case 3:
        if (n == 0) {
          Game.audio.play(SFX_DENY);
          Game.toast("NENHUMA DICA REVELADA AINDA.");
          return;
        }
        Game.audio.play(SFX_SELECT);
        Game.clueJustRevealed = false;
        Game.go(SCR_CLUES, TR_GLITCH);
        break;
      case 4:
        if (toggleResolved()) _stampAt = hal::now();
        else _stampAt = 0;
        break;
    }
  }
  static constexpr int ITEMS = 5;
  int _sel = 0;
  uint32_t _stampAt = 0;
};

// ===========================================================================
// DICAS (passo 3)
// ===========================================================================
class CluesScreen : public Screen {
 public:
  void enter() override { rebuild(Game.clueJustRevealed); }

  void input(const InputEvent& ev) override {
    if (_stamp) {  // pula a animação do carimbo
      endStamp();
      return;
    }
    if (_view.typing()) {
      _view.revealAll();
      Game.redraw();
      return;
    }
    if (_view.handleInput(ev)) {
      Game.redraw();
      return;
    }
    const uint16_t id = Game.caseId();
    const uint8_t n = Game.progress.clues(id);
    if (ev.ch == '1') {
      if (n >= 4) {
        Game.audio.play(SFX_DENY);
        Game.toast("ESTA ERA A ÚLTIMA DICA.");
        return;
      }
      Game.progress.setClues(id, n + 1);
      rebuild(true);
    } else if (ev.ch == '2') {
      Game.audio.play(SFX_SELECT);
      Game.go(SCR_TRUTH, TR_FLASH);
    } else if (ev.key == K_BACK || ev.key == K_ENTER) {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_HUB, TR_GLITCH);
    } else if (ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    }
  }

  bool update(uint32_t now) override {
    if (_stamp) {
      if (now - _t0 > 1150) endStamp();
      return true;
    }
    const bool d = _view.update(now);
    typeTick(_view);
    return d;
  }

  void draw(M5Canvas& c) override {
    const uint8_t n = Game.progress.clues(Game.caseId());
    ui::clear(c);
    if (_stamp) {
      const uint32_t t = hal::now() - _t0;
      ui::alphaMask(c, (SCREEN_W - ART_FINGERPRINT_W) / 2, 20, ART_FINGERPRINT_W, ART_FINGERPRINT_H, ART_FINGERPRINT,
                    C_RED_DARK, 140);
      ui::textCentered(c, F_TITLE, SCREEN_W / 2, 22, "DICA", C_PAPER);
      char big[16];
      snprintf(big, sizeof(big), "%02d / 04", n);
      ui::textCentered(c, F_TITLE_XL, SCREEN_W / 2, 44, big, C_RED);
      const int lw = t > 500 ? 120 : (int)(120 * t / 500);
      c.fillRect(SCREEN_W / 2 - lw / 2, 84, lw, 2, C_RED);
      ui::textCentered(c, F_UI, SCREEN_W / 2, 94, "NOVA EVIDÊNCIA ENCONTRADA", C_GRAY);
      if (t < 320) ui::glitch(c, t * 31u + 7u, 1.0f - t / 320.0f);
      ui::grain(c, t / 50, 160);
      return;
    }
    char right[12];
    snprintf(right, sizeof(right), "DICAS %d/4", n);
    ui::header(c, caseLabel(), right, C_RED);
    _view.draw(c);
    if (n < 4) {
      static const ui::Hint h[] = {{"1", "PRÓXIMA DICA"}, {"2", "VERDADE"}, {"ESC", "VOLTAR"}};
      ui::footer(c, h, 3);
    } else {
      static const ui::Hint h[] = {{"2", "VERDADE"}, {"ESC", "VOLTAR"}, {KEYCAP_UPDOWN, "ROLAR"}};
      ui::footer(c, h, 3);
    }
  }

 private:
  void rebuild(bool animateNewest) {
    const Mystery& m = Game.mystery();
    const uint8_t n = Game.progress.clues(m.id);
    const FontId body = Game.bodyFont();
    _doc.clear();
    _lastRule = 0;
    _lastText = 0;
    for (int i = 0; i < n; ++i) {
      snprintf(_labels[i], sizeof(_labels[i]), "DICA %02d / 04", i + 1);
      const bool newest = i == n - 1;
      _lastRule = _doc.addRule(_labels[i], newest ? C_RED_DARK : C_LINE, newest ? C_RED : C_GRAY, i == 0 ? 0 : 9);
      _lastText = _doc.addText(m.clues[i], body, newest ? C_WHITE : C_PAPER_DIM, 3);
    }
    setupView(_view, _doc);
    _view.scrollToBlock(_lastRule, false);
    _typeFrom = _doc.blockFirstCp(_lastText);
    if (animateNewest && n > 0) {
      _stamp = true;
      _t0 = hal::now();
      Game.audio.play(SFX_CLUE);
      _view.startTyping(_typeFrom, 0xFFFF);  // esconde o texto novo até o carimbo terminar
    } else {
      _stamp = false;
      _view.revealAll();
    }
    Game.clueJustRevealed = false;
    Game.redraw();
  }

  void endStamp() {
    _stamp = false;
    _view.startTyping(_typeFrom, Game.typeMsPerChar());
    Game.redraw();
  }

  Document _doc;
  TextView _view;
  char _labels[4][16];
  int _lastRule = 0, _lastText = 0;
  uint16_t _typeFrom = 0;
  bool _stamp = false;
  uint32_t _t0 = 0;
};

// ===========================================================================
// A VERDADE (passo 4)
// ===========================================================================
class TruthScreen : public Screen {
 public:
  void enter() override {
    const Mystery& m = Game.mystery();
    _t0 = hal::now();
    _intro = true;
    _stampAt = 0;
    Game.audio.stopAll();
    Game.audio.play(SFX_REVEAL);

    _doc.clear();
    _doc.addText(m.title, F_LABEL, C_GRAY, 0);
    _doc.addText("A VERDADE", F_TITLE, C_RED, 1);
    _doc.addRule(nullptr, C_RED_DARK, C_RED, 3);
    _solBlock = _doc.addText(m.solution, Game.bodyFont(), C_PAPER, 4);
    _doc.addSpacer(34);
    setupView(_view, _doc);
    _view.startTyping(_doc.blockFirstCp(_solBlock), 0xFFFF);  // oculto durante a abertura
  }

  void input(const InputEvent& ev) override {
    if (_intro) {
      endIntro();
      return;
    }
    if (_view.typing()) {
      _view.revealAll();
      Game.redraw();
      return;
    }
    if (_view.handleInput(ev)) {
      Game.redraw();
      return;
    }
    if (ev.ch == 'n' || (ev.key == K_ENTER && _view.atBottom())) {
      Game.openCase(Game.nextCaseIndex(Game.caseIndex));
    } else if (ev.key == K_ENTER) {
      _view.scrollTo(_view.scrollY() + _view.h() - 20);
      Game.redraw();
    } else if (ev.ch == 's') {
      _stampAt = toggleResolved() ? hal::now() : 0;
      if (_stampAt) _view.scrollTo(_view.document()->height());  // mostra o carimbo
    } else if (ev.ch == 'r') {
      Game.progress.restartCase(Game.caseId());
      Game.openCase(Game.caseIndex);
    } else if (ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    } else if (ev.key == K_BACK) {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_HUB, TR_GLITCH);
    }
  }

  bool update(uint32_t now) override {
    if (_intro) {
      if (now - _t0 > 1500) endIntro();
      return true;
    }
    bool d = _view.update(now);
    typeTick(_view);
    // casos já marcados como resolvidos mostram o carimbo (sem som) ao fim do texto
    if (!_view.typing() && _stampAt == 0 && Game.progress.resolved(Game.caseId())) {
      _stampAt = now;
      d = true;
    }
    if (_stampAt && now - _stampAt < 400) d = true;  // animação do carimbo
    return d;
  }

  void draw(M5Canvas& c) override {
    const uint32_t now = hal::now();
    ui::clear(c);
    if (_intro) {
      const uint32_t t = now - _t0;
      ui::alphaMask(c, 156, 22, ART_FINGERPRINT_W, ART_FINGERPRINT_H, ART_FINGERPRINT, C_RED_DARK, 150);
      static const char* T = "A VERDADE";
      char buf[16];
      int n = (int)(t / 85);
      if (n > 9) n = 9;
      memcpy(buf, T, n);
      buf[n] = 0;
      const int w = Fonts::textWidth(F_TITLE_XL, T);
      ui::text(c, F_TITLE_XL, (SCREEN_W - w) / 2, 46, buf, C_RED);
      if (t > 800) {
        const int lw = t > 1100 ? 160 : (int)(160 * (t - 800) / 300);
        c.fillRect(SCREEN_W / 2 - lw / 2, 84, lw, 2, C_RED);
      }
      ui::grain(c, t / 45, 220);
      if (t < 250 || (t > 700 && t < 780)) ui::glitch(c, t * 13u + 1u, 0.9f);
      return;
    }
    ui::header(c, caseLabel(), "A VERDADE", C_RED);
    _view.draw(c);
    // carimbo no fim do documento
    if (_stampAt && Game.progress.resolved(Game.caseId())) {
      const int y = _view.y() + _view.document()->height() - 30 - _view.scrollY();
      if (y > _view.y() - 26 && y < _view.y() + _view.h()) {
        c.setClipRect(_view.x(), _view.y(), _view.w() + 1, _view.h());
        const uint32_t st = now - _stampAt;
        const int jitter = st < 160 ? (int)((160 - st) / 40) : 0;
        ui::stamp(c, _view.x() + 60 + jitter, y + 2 - jitter, "RESOLVIDO", C_RED, Game.caseId() * 977u);
        c.clearClipRect();
      }
    }
    if (Game.progress.resolved(Game.caseId())) {
      static const ui::Hint h[] = {{"N", "PRÓXIMO"}, {"S", "REABRIR"}, {"M", "MENU"}, {KEYCAP_UPDOWN, "ROLAR"}};
      ui::footer(c, h, 4);
    } else {
      static const ui::Hint h[] = {{"S", "RESOLVIDO"}, {"N", "PRÓXIMO"}, {"M", "MENU"}, {KEYCAP_UPDOWN, "ROLAR"}};
      ui::footer(c, h, 4);
    }
  }

 private:
  void endIntro() {
    _intro = false;
    _view.startTyping(_doc.blockFirstCp(_solBlock), Game.typeMsPerChar());
    Game.redraw();
  }
  Document _doc;
  TextView _view;
  int _solBlock = 0;
  bool _intro = false;
  uint32_t _t0 = 0, _stampAt = 0;
};

IntroScreen s_intro;
HubScreen s_hub;
CluesScreen s_clues;
TruthScreen s_truth;

}  // namespace

namespace screens {
Screen* intro() { return &s_intro; }
Screen* hub() { return &s_hub; }
Screen* clues() { return &s_clues; }
Screen* truth() { return &s_truth; }
}  // namespace screens
