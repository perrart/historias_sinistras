// Abertura, menu principal, "Como jogar" e créditos.
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

// Conta codepoints UTF-8
int cpCount(const char* s) {
  int n = 0;
  for (; *s; ++s)
    if ((*s & 0xC0) != 0x80) ++n;
  return n;
}

// Copia os primeiros n codepoints de s para out
void cpPrefix(const char* s, int n, char* out, size_t outSize) {
  size_t i = 0;
  int count = 0;
  while (s[i] && i + 1 < outSize) {
    if ((s[i] & 0xC0) != 0x80) {
      if (count == n) break;
      ++count;
    }
    out[i] = s[i];
    ++i;
  }
  out[i] = 0;
}

// ===========================================================================
// ABERTURA
// ===========================================================================
class SplashScreen : public Screen {
 public:
  void enter() override {
    _t0 = hal::now();
    _lastLetters = 0;
    _frameKey = 0xFFFFFFFF;
    Game.audio.play(SFX_BOOT);
  }

  void input(const InputEvent& ev) override {
    (void)ev;
    const uint32_t t = hal::now() - _t0;
    if (t < 3000) {
      _t0 = hal::now() - 3000;  // pula a animação
      Game.redraw();
      return;
    }
    Game.audio.play(SFX_SELECT);
    Game.go(SCR_MENU, TR_GLITCH);
  }

  bool update(uint32_t now) override {
    const uint32_t t = now - _t0;
    if (t > 12000) {
      Game.go(SCR_MENU, TR_GLITCH);
      return false;
    }
    const int letters = lettersAt(t);
    if (letters > _lastLetters && t < 3000) {
      Game.audio.play(SFX_TYPE);
    }
    _lastLetters = letters;
    // redesenha a ~25 fps durante a animação e a cada mudança depois
    const uint32_t key = t < 3400 ? t / 40 : (t / 300) + 100000;
    if (key != _frameKey) {
      _frameKey = key;
      return true;
    }
    return false;
  }

  void draw(M5Canvas& c) override {
    const uint32_t t = hal::now() - _t0;
    ui::clear(c);

    // impressão digital em vermelho, surgindo devagar
    const float fp = t > 1600 ? 1.0f : t / 1600.0f;
    ui::alphaMask(c, 156, 22, ART_FINGERPRINT_W, ART_FINGERPRINT_H, ART_FINGERPRINT, C_RED, (uint8_t)(fp * 115));

    // títulos datilografados
    static const char* L1 = "HISTÓRIAS";
    static const char* L2 = "SINISTRAS";
    char buf[32];
    const int n1 = cpCount(L1);
    const int letters = lettersAt(t);
    cpPrefix(L1, letters, buf, sizeof(buf));
    ui::text(c, F_TITLE_XL, 12, 12, buf, C_PAPER);
    if (letters > n1) {
      cpPrefix(L2, letters - n1, buf, sizeof(buf));
      ui::text(c, F_TITLE_XL, 12, 44, buf, C_RED);
    }

    // linha vermelha
    if (t > 1750) {
      const float k = t > 2150 ? 1.0f : (t - 1750) / 400.0f;
      c.fillRect(13, 82, (int)(150 * k), 2, C_RED);
      c.fillRect(13 + (int)(150 * k), 82, 2, 2, C_RED_GLOW);
    }

    // frase
    static const char* TAG = "TODO SEGREDO TEM UMA HISTÓRIA.";
    if (t > 1950) {
      int n = (int)((t - 1950) / 28);
      cpPrefix(TAG, n, buf, sizeof(buf));
      ui::text(c, F_UI, 13, 89, buf, C_PAPER_DIM);
    }

    // convite piscante
    if (t > 3000 && ((t / 650) % 2 == 0)) {
      ui::textCentered(c, F_UI, SCREEN_W / 2, 117, "PRESSIONE ENTER", C_GRAY);
    }
    ui::textRight(c, F_UI, SCREEN_W - 3, 123, "v" HS_VERSION, C_GRAY_DARK);

    ui::grain(c, t / 80, 140);
    // interferências ocasionais
    const bool burst = (t > 230 && t < 320) || (t > 1820 && t < 1900) || (t > 3500 && (t % 2700) < 90);
    if (burst) ui::glitch(c, t * 7919u, 0.7f);
  }

 private:
  static int lettersAt(uint32_t t) {
    if (t < 350) return 0;
    const int a = (int)((t - 350) / 72) + 1;
    if (a <= 9) return a;
    if (t < 1100) return 9;
    const int b = (int)((t - 1100) / 72) + 1;
    return 9 + (b > 9 ? 9 : b);
  }
  uint32_t _t0 = 0;
  int _lastLetters = 0;
  uint32_t _frameKey = 0;
};

// ===========================================================================
// MENU PRINCIPAL
// ===========================================================================
class MenuScreen : public Screen {
 public:
  void enter() override {
    _t0 = hal::now();
    _frameKey = 0xFFFFFFFF;
    _resolved = Game.resolvedCount();
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
    } else if (ev.ch >= '1' && ev.ch <= '0' + ITEMS) {
      _sel = ev.ch - '1';
      activate();
    } else if (ev.key == K_ENTER) {
      activate();
    } else if (ev.key == K_BACK) {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_SPLASH, TR_GLITCH);
    }
  }

  bool update(uint32_t now) override {
    const uint32_t key = (now - _t0) / 60;  // anima o olho
    if (key != _frameKey) {
      _frameKey = key;
      return true;
    }
    return false;
  }

  void draw(M5Canvas& c) override {
    const uint32_t t = hal::now() - _t0;
    ui::clear(c);

    // título
    Fonts::use(c, F_TITLE);
    c.setTextColor(C_PAPER);
    c.drawString("HISTÓRIAS", 10, 3);
    c.setTextColor(C_RED);
    c.drawString("SINISTRAS", 10 + Fonts::textWidth(F_TITLE, "HISTÓRIAS "), 3);
    c.drawFastHLine(10, 24, 220, C_LINE);
    c.drawFastHLine(10, 24, 48, C_RED);

    static const char* items[ITEMS] = {"JOGAR",      "ESCOLHER CASO", "CASOS RESOLVIDOS",
                                       "COMO JOGAR", "CONFIGURAÇÕES", "CRÉDITOS"};
    for (int i = 0; i < ITEMS; ++i) {
      const int y = 28 + i * 15;
      const bool on = i == _sel;
      char num[2] = {(char)('1' + i), 0};
      if (on) {
        c.fillRect(6, y, 146, 15, C_CHARCOAL2);
        const uint8_t glow = (uint8_t)(128 + 127 * sinf(t / 260.0f));
        c.fillRect(6, y, 3, 15, ui::blend(C_RED_DARK, C_RED_GLOW, glow));
      }
      ui::text(c, F_UI, 14, y + 2, num, on ? C_RED : C_GRAY_DARK);
      ui::text(c, F_BODY, 25, y, items[i], on ? C_WHITE : C_PAPER_DIM);
    }

    // painel lateral
    c.drawFastVLine(157, 30, 86, C_LINE);
    const bool blink = (t % 5200) < 140;
    if (blink) {
      c.drawFastHLine(181, 40, 28, C_RED);
      c.drawFastHLine(183, 41, 24, C_RED_DARK);
    } else {
      const uint8_t op = (uint8_t)(170 + 85 * sinf(t / 600.0f));
      ui::alphaMask(c, 180, 32, ART_EYE_W, ART_EYE_H, ART_EYE, C_RED, op);
    }
    char num[8];
    snprintf(num, sizeof(num), "%02d", _resolved);
    const int nw = Fonts::textWidth(F_TITLE_XL, num);
    char tot[8];
    snprintf(tot, sizeof(tot), "/%d", MysteryDB::count());
    const int tw = Fonts::textWidth(F_TITLE, tot);
    const int x0 = 196 - (nw + tw + 2) / 2;
    ui::text(c, F_TITLE_XL, x0, 52, num, C_RED);
    ui::text(c, F_TITLE, x0 + nw + 2, 64, tot, C_GRAY);
    ui::textCentered(c, F_UI, 196, 86, "RESOLVIDOS", C_GRAY);
    ui::progressBar(c, 164, 101, 64, 5, _resolved, MysteryDB::count());
    ui::battery(c, 189, 110, hal::batteryLevel(), hal::charging());

    static const ui::Hint hints[] = {{KEYCAP_UPDOWN, "NAVEGAR"}, {"ENTER", "ABRIR"}, {"1-6", "ATALHO"}};
    ui::footer(c, hints, 3);
  }

 private:
  void activate() {
    Game.audio.play(SFX_SELECT);
    switch (_sel) {
      case 0: Game.caseReturn = SCR_MENU; Game.startPlay(); break;
      case 1: Game.go(SCR_BROWSER, TR_GLITCH); break;
      case 2: Game.go(SCR_RESOLVED, TR_GLITCH); break;
      case 3: Game.go(SCR_HOWTO, TR_GLITCH); break;
      case 4: Game.go(SCR_SETTINGS, TR_GLITCH); break;
      case 5: Game.go(SCR_CREDITS, TR_GLITCH); break;
    }
  }
  static constexpr int ITEMS = 6;
  int _sel = 0;
  uint32_t _t0 = 0;
  uint32_t _frameKey = 0;
  int _resolved = 0;
};

// ===========================================================================
// COMO JOGAR
// ===========================================================================
class HowToScreen : public Screen {
 public:
  void enter() override {
    const FontId body = Game.bodyFont();
    _doc.clear();
    _doc.addText("CADA ARQUIVO GUARDA UMA HISTÓRIA SINISTRA.", F_LABEL, C_RED, 0);
    _doc.addText(
        "Mortes estranhas, crimes impossíveis, desaparecimentos. Você lê o que aconteceu e precisa "
        "descobrir o que realmente aconteceu. A resposta é lógica — basta enxergar o detalhe certo.",
        body, C_PAPER, 3);
    _doc.addRule("JOGANDO SOZINHO", C_LINE, C_RED, 8);
    _doc.addText(
        "Leia o cenário e monte uma teoria. Se travar, revele as dicas uma a uma com a tecla [1]: cada "
        "uma aproxima você da verdade. Quando tiver certeza, revele a verdade com [2]. O desafio é "
        "acertar usando o menor número de dicas. Depois de ler a verdade, use MARCAR COMO RESOLVIDO "
        "no menu do caso (ou a tecla [S]) se você acertou.",
        body, C_PAPER, 3);
    _doc.addRule("JOGANDO EM GRUPO", C_LINE, C_RED, 8);
    _doc.addText(
        "Um jogador é o Guardião do Segredo: ele abre a verdade escondido dos outros e lê em silêncio. "
        "Os detetives fazem perguntas que só podem ser respondidas com SIM, NÃO ou IRRELEVANTE. Quando "
        "o grupo empacar, o Guardião revela uma dica em voz alta. Vence quem explicar a história "
        "inteira.",
        body, C_PAPER, 3);
    _doc.addRule("CONTROLES", C_LINE, C_RED, 8);
    _doc.addText(
        "Setas ; e . — navegar e rolar o texto\n"
        "Setas , e / — página anterior / próxima\n"
        "ENTER ou G0 — confirmar\n"
        "ESC ( ` ) ou APAGAR — voltar\n"
        "1 — revelar a próxima dica\n"
        "2 — revelar a verdade\n"
        "S — marcar / desmarcar como resolvido\n"
        "N — próximo caso   M — menu",
        body, C_PAPER, 3);
    _doc.addRule("DIFICULDADE", C_LINE, C_RED, 8);
    _doc.addText(
        "FÁCIL, MÉDIO, DIFÍCIL e INSANO. Os quadradinhos vermelhos no topo de cada caso mostram o "
        "nível. Casos marcados como resolvidos aparecem no arquivo e podem ser jogados de novo.",
        body, C_PAPER, 3);
    _doc.addCenter("— FIM DAS INSTRUÇÕES —", F_UI, C_GRAY_DARK, 10);
    _view.setViewport(MARGIN_X, CONTENT_Y, SCREEN_W - 2 * MARGIN_X - 4, CONTENT_H);
    _doc.layout(_view.w());
    _view.setDocument(&_doc);
  }

  void input(const InputEvent& ev) override {
    if (_view.handleInput(ev)) {
      Game.redraw();
      return;
    }
    if (ev.key == K_BACK || ev.key == K_ENTER || ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    }
  }

  bool update(uint32_t now) override { return _view.update(now); }

  void draw(M5Canvas& c) override {
    ui::clear(c);
    ui::header(c, "COMO JOGAR", "MANUAL DO DETETIVE");
    _view.draw(c);
    static const ui::Hint hints[] = {{KEYCAP_UPDOWN, "ROLAR"}, {KEYCAP_LEFTRIGHT, "PÁGINA"}, {"ESC", "VOLTAR"}};
    ui::footer(c, hints, 3);
  }

 private:
  Document _doc;
  TextView _view;
};

// ===========================================================================
// CRÉDITOS
// ===========================================================================
class CreditsScreen : public Screen {
 public:
  void input(const InputEvent& ev) override {
    if (ev.key == K_BACK || ev.key == K_ENTER || ev.ch == 'm') {
      Game.audio.play(SFX_BACK);
      Game.go(SCR_MENU, TR_GLITCH);
    }
  }

  void draw(M5Canvas& c) override {
    ui::clear(c);
    ui::header(c, "CRÉDITOS", "HISTÓRIAS SINISTRAS v" HS_VERSION);

    // coluna de texto
    ui::text(c, F_UI, MARGIN_X, 18, "JOGO CRIADO POR", C_GRAY);
    ui::text(c, F_TITLE, MARGIN_X, 30, "@Perrart", C_WHITE);
    c.fillRect(MARGIN_X, 53, 44, 2, C_RED);
    ui::text(c, F_UI, MARGIN_X, 58, "CÓDIGO E MAIS JOGOS", C_GRAY);
    ui::text(c, F_LABEL, MARGIN_X, 70, "github.com/", C_PAPER);
    ui::text(c, F_LABEL, MARGIN_X, 83, "perrart/cardputer", C_PAPER);
    ui::text(c, F_UI, MARGIN_X, 103, "ESCANEIE O QR CODE", C_RED);

    // QR code com o link (maiúsculas = modo alfanumérico, código menor e mais legível)
    c.qrcode("HTTPS://GITHUB.COM/PERRART/CARDPUTER", QR_X, QR_Y, QR_W, 1);

    static const ui::Hint hints[] = {{"ESC", "VOLTAR"}};
    ui::footer(c, hints, 1);
  }

 private:
  static constexpr int QR_W = 99, QR_X = SCREEN_W - MARGIN_X - QR_W + 2, QR_Y = 19;
};

SplashScreen s_splash;
MenuScreen s_menu;
HowToScreen s_howto;
CreditsScreen s_credits;

}  // namespace

namespace screens {
Screen* splash() { return &s_splash; }
Screen* menu() { return &s_menu; }
Screen* howTo() { return &s_howto; }
Screen* credits() { return &s_credits; }

Screen* get(ScreenId id) {
  switch (id) {
    case SCR_SPLASH: return splash();
    case SCR_MENU: return menu();
    case SCR_INTRO: return intro();
    case SCR_HUB: return hub();
    case SCR_CLUES: return clues();
    case SCR_TRUTH: return truth();
    case SCR_BROWSER: return browser();
    case SCR_RESOLVED: return resolved();
    case SCR_HOWTO: return howTo();
    case SCR_SETTINGS: return settings();
    case SCR_RESET: return reset();
    case SCR_CREDITS: return credits();
    default: return menu();
  }
}
}  // namespace screens
