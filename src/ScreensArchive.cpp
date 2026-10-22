// Arquivo de casos (navegador), casos resolvidos, configurações e reinício de progresso.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Config.h"
#include "Platform.h"
#include "ScreenManager.h"
#include "Screens.h"
#include "StoryRenderer.h"

using namespace cfg;

namespace {

// ---------------------------------------------------------------------------
// Lista rolável de casos (compartilhada entre o arquivo e os resolvidos)
// ---------------------------------------------------------------------------
class CaseList {
 public:
  static constexpr int MAX = 255;
  void setAll() {
    _count = MysteryDB::count();
    if (_count > MAX) _count = MAX;
    for (int i = 0; i < _count; ++i) _idx[i] = (uint8_t)i;
  }
  void setResolved() {
    _count = 0;
    for (int i = 0; i < MysteryDB::count() && _count < MAX; ++i)
      if (Game.progress.resolved(MysteryDB::at(i).id)) _idx[_count++] = (uint8_t)i;
  }
  int count() const { return _count; }
  int selectedIndex() const { return _count ? _idx[_sel] : -1; }
  void selectCaseIndex(int caseIdx) {
    for (int i = 0; i < _count; ++i)
      if (_idx[i] == caseIdx) { _sel = i; break; }
    clamp();
  }
  void setRows(int rows) { _rows = rows; clamp(); }
  bool move(int delta) {
    if (!_count) return false;
    int s = _sel + delta;
    if (s < 0) s = (delta < -1) ? 0 : _count - 1;           // seta para cima no topo: vai ao fim
    if (s >= _count) s = (delta > 1) ? _count - 1 : 0;       // página no fim: para no último
    const bool changed = s != _sel;
    _sel = s;
    clamp();
    return changed;
  }
  void draw(M5Canvas& c, int y0, int rowH) {
    for (int r = 0; r < _rows; ++r) {
      const int i = _top + r;
      if (i >= _count) break;
      const Mystery& m = MysteryDB::at(_idx[i]);
      const int y = y0 + r * rowH;
      const bool on = i == _sel;
      if (on) {
        c.fillRect(4, y, SCREEN_W - 8, rowH, C_CHARCOAL2);
        c.fillRect(4, y, 3, rowH, C_RED);
      }
      char num[6];
      snprintf(num, sizeof(num), "%03u", (unsigned)m.id);
      ui::text(c, F_UI, 11, y + 1, num, on ? C_RED : C_GRAY);
      const CaseStatus st = Game.progress.status(m.id);
      const uint16_t col = on ? C_WHITE : (st == STATUS_RESOLVED ? C_PAPER_DIM : C_PAPER);
      ui::text(c, F_LABEL, 36, y, m.title, col);
      ui::statusIcon(c, SCREEN_W - 22, y + (rowH - 9) / 2, st, Game.progress.clues(m.id));
    }
    // barra de rolagem
    if (_count > _rows) {
      const int h = _rows * rowH;
      const int th = h * _rows / _count < 6 ? 6 : h * _rows / _count;
      const int ty = y0 + (h - th) * _top / (_count - _rows);
      c.drawFastVLine(SCREEN_W - 3, y0, h, C_LINE);
      c.fillRect(SCREEN_W - 4, ty, 3, th, C_RED);
    }
  }

 private:
  void clamp() {
    if (_sel >= _count) _sel = _count ? _count - 1 : 0;
    if (_sel < 0) _sel = 0;
    if (_sel < _top) _top = _sel;
    if (_sel >= _top + _rows) _top = _sel - _rows + 1;
    if (_top < 0) _top = 0;
  }
  uint8_t _idx[MAX];
  int _count = 0;
  int _sel = 0;
  int _top = 0;
  int _rows = 6;
};

// ===========================================================================
// ARQUIVO DE CASOS
// ===========================================================================
class BrowserScreen : public Screen {
 public:
  void enter() override {
    _list.setAll();
    _list.setRows(ROWS);
    _list.selectCaseIndex(Game.browserSelection);
    _digits[0] = 0;
    _resolved = Game.resolvedCount();
  }

  void input(const InputEvent& ev) override {
    bool moved = false;
    if (ev.key == K_UP) moved = _list.move(-1);
    else if (ev.key == K_DOWN) moved = _list.move(1);
    else if (ev.key == K_LEFT) moved = _list.move(-ROWS);
    else if (ev.key == K_RIGHT || ev.key == K_SPACE) moved = _list.move(ROWS);
    else if (ev.ch >= '0' && ev.ch <= '9') {
      typeDigit(ev.ch);
      return;
    } else if (ev.key == K_ENTER) {
      const int idx = _list.selectedIndex();
      if (idx >= 0) {
        Game.caseReturn = SCR_BROWSER;
        Game.openCase(idx);
      }
      return;
    } else if (ev.key == K_BACK || ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
      return;
    }
    if (moved) {
      Game.browserSelection = _list.selectedIndex();
      Game.audio.play(SFX_NAV);
      Game.redraw();
    }
  }

  bool update(uint32_t now) override {
    if (_digits[0] && now - _digitAt > 1100) {
      _digits[0] = 0;
      return true;
    }
    return false;
  }

  void draw(M5Canvas& c) override {
    ui::clear(c);
    char right[16];
    snprintf(right, sizeof(right), "%02d/%d", _resolved, MysteryDB::count());
    ui::header(c, "ARQUIVO DE CASOS", right, C_RED);
    _list.draw(c, 16, ROW_H);

    // detalhes do caso selecionado
    const int idx = _list.selectedIndex();
    const int dy = 16 + ROWS * ROW_H + 1;
    c.drawFastHLine(4, dy, SCREEN_W - 8, C_LINE);
    if (idx >= 0) {
      const Mystery& m = MysteryDB::at(idx);
      ui::text(c, F_UI, 8, dy + 2, MysteryDB::categoryName(m.category), C_PAPER_DIM);
      const char* dn = MysteryDB::difficultyName(m.difficulty);
      ui::textRight(c, F_UI, SCREEN_W - 8, dy + 2, dn, m.difficulty >= DIFF_DIFICIL ? C_RED : C_GRAY);
      ui::difficultyPips(c, SCREEN_W - 8 - Fonts::textWidth(F_UI, dn) - 28, dy + 6, m.difficulty);
    }
    if (_digits[0]) {
      char msg[24];
      snprintf(msg, sizeof(msg), "IR PARA O CASO %s_", _digits);
      ui::toast(c, msg);
    }
    static const ui::Hint h[] = {{KEYCAP_UPDOWN, "MOVER"}, {"0-9", "Nº"}, {"ENTER", "ABRIR"}, {"ESC", "SAIR"}};
    ui::footer(c, h, 4);
  }

 private:
  static constexpr int ROWS = 6;
  static constexpr int ROW_H = 15;

  void typeDigit(char d) {
    const uint32_t now = hal::now();
    size_t len = strlen(_digits);
    if (len >= 3 || now - _digitAt > 1100) len = 0;
    _digits[len] = d;
    _digits[len + 1] = 0;
    _digitAt = now;
    const int id = atoi(_digits);
    const int idx = MysteryDB::indexOfId((uint16_t)id);
    if (idx >= 0) {
      _list.selectCaseIndex(idx);
      Game.browserSelection = idx;
    }
    Game.audio.play(SFX_NAV);
    Game.redraw();
  }

  CaseList _list;
  char _digits[4] = {0};
  uint32_t _digitAt = 0;
  int _resolved = 0;
};

// ===========================================================================
// CASOS RESOLVIDOS
// ===========================================================================
class ResolvedScreen : public Screen {
 public:
  void enter() override {
    _list.setResolved();
    _list.setRows(ROWS);
  }

  void input(const InputEvent& ev) override {
    bool moved = false;
    if (ev.key == K_UP) moved = _list.move(-1);
    else if (ev.key == K_DOWN) moved = _list.move(1);
    else if (ev.key == K_LEFT) moved = _list.move(-ROWS);
    else if (ev.key == K_RIGHT) moved = _list.move(ROWS);
    else if (ev.key == K_ENTER) {
      const int idx = _list.selectedIndex();
      if (idx >= 0) {
        Game.caseReturn = SCR_RESOLVED;
        Game.openCase(idx);
      } else {
        Game.audio.play(SFX_DENY);
      }
      return;
    } else if (ev.key == K_BACK || ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
      return;
    }
    if (moved) {
      Game.audio.play(SFX_NAV);
      Game.redraw();
    }
  }

  void draw(M5Canvas& c) override {
    const int total = MysteryDB::count();
    const int solved = _list.count();
    ui::clear(c);
    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", total ? solved * 100 / total : 0);
    ui::header(c, "CASOS RESOLVIDOS", pct, C_RED);

    char num[8], tot[8];
    snprintf(num, sizeof(num), "%02d", solved);
    snprintf(tot, sizeof(tot), "/ %d", total);
    ui::text(c, F_TITLE_XL, 10, 14, num, C_RED);
    ui::text(c, F_TITLE, 12 + Fonts::textWidth(F_TITLE_XL, num), 25, tot, C_GRAY);

    // por dificuldade
    int done[DIFF_COUNT] = {0}, all[DIFF_COUNT] = {0};
    for (int i = 0; i < total; ++i) {
      const Mystery& m = MysteryDB::at(i);
      if (m.difficulty < DIFF_COUNT) {
        ++all[m.difficulty];
        if (Game.progress.resolved(m.id)) ++done[m.difficulty];
      }
    }
    for (int d = 0; d < DIFF_COUNT; ++d) {
      const int x = 92 + (d % 2) * 72;
      const int y = 17 + (d / 2) * 15;
      char s[20];
      snprintf(s, sizeof(s), "%d/%d", done[d], all[d]);
      ui::text(c, F_LABEL, x, y, MysteryDB::difficultyName(d), C_GRAY);
      ui::textRight(c, F_LABEL, x + 67, y, s, done[d] == all[d] && all[d] ? C_RED : C_PAPER);
    }
    ui::progressBar(c, 10, 50, SCREEN_W - 20, 6, solved, total);

    if (!solved) {
      ui::textCentered(c, F_LABEL, SCREEN_W / 2, 72, "NENHUM CASO RESOLVIDO AINDA.", C_PAPER);
      ui::textCentered(c, F_UI, SCREEN_W / 2, 90, "ABRA UM ARQUIVO E DESCUBRA A VERDADE.", C_GRAY);
    } else {
      _list.draw(c, 60, ROW_H);
    }
    static const ui::Hint h[] = {{KEYCAP_UPDOWN, "NAVEGAR"}, {"ENTER", "REVER CASO"}, {"ESC", "VOLTAR"}};
    ui::footer(c, h, 3);
  }

 private:
  static constexpr int ROWS = 4;
  static constexpr int ROW_H = 15;
  CaseList _list;
};

// ===========================================================================
// CONFIGURAÇÕES
// ===========================================================================
class SettingsScreen : public Screen {
 public:
  void enter() override {
    if (Game.previous() != SCR_RESET) _sel = 0;  // volta ao item ao sair da confirmação
  }

  void leave() override { Game.settings.save(); }

  void input(const InputEvent& ev) override {
    if (ev.key == K_UP) {
      _sel = (_sel + ITEMS - 1) % ITEMS;
      Game.audio.play(SFX_NAV);
    } else if (ev.key == K_DOWN) {
      _sel = (_sel + 1) % ITEMS;
      Game.audio.play(SFX_NAV);
    } else if (ev.key == K_LEFT) {
      change(-1);
    } else if (ev.key == K_RIGHT || ev.key == K_ENTER || ev.key == K_SPACE) {
      change(+1);
    } else if (ev.key == K_BACK || ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
      return;
    }
    Game.redraw();
  }

  void draw(M5Canvas& c) override {
    const SettingsData& s = Game.settings.data();
    ui::clear(c);
    ui::header(c, "CONFIGURAÇÕES", "v" HS_VERSION);
    static const char* labels[ITEMS] = {"SOM", "VOLUME", "VELOCIDADE DO TEXTO", "TAMANHO DO TEXTO", "BRILHO",
                                        "ZERAR PROGRESSO"};
    static const char* vol[3] = {"BAIXO", "MÉDIO", "ALTO"};
    static const char* spd[3] = {"NORMAL", "RÁPIDA", "INSTANTÂNEA"};
    static const char* siz[2] = {"NORMAL", "GRANDE"};
    for (int i = 0; i < ITEMS; ++i) {
      const int y = 18 + i * 17;
      const bool on = i == _sel;
      if (on) {
        c.fillRect(4, y - 1, SCREEN_W - 8, 16, C_CHARCOAL2);
        c.fillRect(4, y - 1, 3, 16, C_RED);
      }
      const bool danger = i == 5;
      ui::text(c, F_LABEL, 12, y, labels[i], danger ? (on ? C_RED_GLOW : C_RED) : (on ? C_WHITE : C_PAPER_DIM));
      const char* v = nullptr;
      switch (i) {
        case 0: v = s.sound ? "LIGADO" : "DESLIGADO"; break;
        case 1: v = vol[s.volume % 3]; break;
        case 2: v = spd[s.textSpeed % 3]; break;
        case 3: v = siz[s.textSize % 2]; break;
        case 4: break;
        case 5: v = "APAGAR"; break;
      }
      const int rx = SCREEN_W - 14;
      int vx;
      if (i == 4) {  // barras de brilho
        for (int b = 0; b < 4; ++b) {
          const int bx = rx - 4 * 8 + b * 8 + 2;
          if (b <= s.brightness) c.fillRect(bx, y + 9 - b * 2, 5, 4 + b * 2, on ? C_RED : C_PAPER_DIM);
          else c.drawRect(bx, y + 9 - b * 2, 5, 4 + b * 2, C_GRAY_DARK);
        }
        vx = rx - 32;
      } else {
        const int vw = Fonts::textWidth(F_LABEL, v);
        vx = rx - vw;
        ui::text(c, F_LABEL, vx, y, v, on ? C_WHITE : (i == 0 && !s.sound ? C_GRAY : C_PAPER));
      }
      if (on && i != 5) {
        c.fillTriangle(vx - 9, y + 7, vx - 5, y + 4, vx - 5, y + 10, C_RED);
        c.fillTriangle(rx + 9, y + 7, rx + 5, y + 4, rx + 5, y + 10, C_RED);
      }
    }
    static const ui::Hint h[] = {{KEYCAP_UPDOWN, "ITEM"}, {KEYCAP_LEFTRIGHT, "ALTERAR"}, {"ESC", "SALVAR"}};
    ui::footer(c, h, 3);
  }

 private:
  static constexpr int ITEMS = 6;
  void change(int dir) {
    SettingsData& s = Game.settings.data();
    switch (_sel) {
      case 0: s.sound = s.sound ? 0 : 1; break;
      case 1: s.volume = (uint8_t)((s.volume + 3 + dir) % 3); break;
      case 2: s.textSpeed = (uint8_t)((s.textSpeed + 3 + dir) % 3); break;
      case 3: s.textSize = (uint8_t)((s.textSize + 2 + dir) % 2); break;
      case 4: s.brightness = (uint8_t)((s.brightness + 4 + dir) % 4); break;
      case 5:
        Game.audio.play(SFX_SELECT);
        Game.go(SCR_RESET, TR_GLITCH);
        return;
    }
    Game.settings.markDirty();
    Game.applySettings();
    Game.audio.play(SFX_SELECT);
  }
  int _sel = 0;
};

// ===========================================================================
// ZERAR PROGRESSO
// ===========================================================================
class ResetScreen : public Screen {
 public:
  void enter() override {
    _t0 = hal::now();
    _doc.clear();
    _doc.addCenter("Todos os casos voltarão a ficar sem solução e as dicas reveladas serão esquecidas. "
                   "Esta ação não pode ser desfeita.",
                   F_BODY, C_PAPER, 0);
    _view.setViewport(14, 47, SCREEN_W - 28, 74);
    _doc.layout(_view.w());
    _view.setDocument(&_doc);
    Game.audio.play(SFX_DENY);
  }

  void input(const InputEvent& ev) override {
    if (ev.key == K_ENTER) {
      if (hal::now() - _t0 < 700) return;  // segurança contra toque duplo
      Game.progress.resetAll();
      Game.progress.save();
      Game.caseIndex = 0;
      Game.browserSelection = 0;
      Game.audio.play(SFX_REVEAL);
      Game.toast("PROGRESSO APAGADO.");
      Game.go(SCR_SETTINGS, TR_FLASH);
    } else if (ev.key == K_BACK || ev.ch == 'n') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_SETTINGS, TR_GLITCH);
    }
  }

  void draw(M5Canvas& c) override {
    ui::clear(c);
    ui::vignette(c, C_RED, 150);
    ui::header(c, "CONFIGURAÇÕES", "ATENÇÃO", C_RED);
    ui::textCentered(c, F_TITLE, SCREEN_W / 2, 23, "ZERAR PROGRESSO?", C_RED);
    _view.draw(c);
    static const ui::Hint h[] = {{"ENTER", "APAGAR TUDO"}, {"ESC", "CANCELAR"}};
    ui::footer(c, h, 2);
  }

 private:
  Document _doc;
  TextView _view;
  uint32_t _t0 = 0;
};

BrowserScreen s_browser;
ResolvedScreen s_resolved;
SettingsScreen s_settings;
ResetScreen s_reset;

}  // namespace

namespace screens {
Screen* browser() { return &s_browser; }
Screen* resolved() { return &s_resolved; }
Screen* settings() { return &s_settings; }
Screen* reset() { return &s_reset; }
}  // namespace screens
