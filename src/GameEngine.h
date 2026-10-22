// Máquina de estados principal (não bloqueante). Cada tela é um estado com
// enter/input/update/draw; o motor cuida de transições, redesenho sob demanda,
// economia de tela e persistência.
#pragma once
#include <stdint.h>
#include <M5GFX.h>
#include "AudioManager.h"
#include "Fonts.h"
#include "InputManager.h"
#include "MysteryDatabase.h"
#include "ProgressManager.h"
#include "SettingsManager.h"

enum ScreenId : uint8_t {
  SCR_SPLASH = 0,
  SCR_MENU,
  SCR_INTRO,      // apresentação do caso (cenário)
  SCR_HUB,        // investigação
  SCR_CLUES,      // dicas reveladas
  SCR_TRUTH,      // a verdade
  SCR_BROWSER,    // arquivo de casos
  SCR_RESOLVED,   // casos resolvidos
  SCR_HOWTO,      // como jogar
  SCR_SETTINGS,   // configurações
  SCR_RESET,      // confirmação para zerar progresso
  SCR_CREDITS,    // créditos
  SCR_COUNT
};

enum Transition : uint8_t { TR_NONE = 0, TR_GLITCH, TR_WIPE, TR_FLASH };

class Screen {
 public:
  virtual ~Screen() {}
  virtual void enter() {}
  virtual void leave() {}
  virtual void input(const InputEvent& ev) = 0;
  virtual bool update(uint32_t now) { (void)now; return false; }  // true = redesenhar
  virtual void draw(M5Canvas& c) = 0;
};

class GameEngine {
 public:
  void begin();
  void tick();

  void go(ScreenId id, Transition t = TR_GLITCH);
  ScreenId current() const { return _currentId; }
  ScreenId previous() const { return _prevId; }
  void redraw() { _dirty = true; }
  void toast(const char* msg, uint32_t ms = 1700);

  // ---- Fluxo de casos ------------------------------------------------------
  void openCase(int index);                 // abre o caso pela apresentação
  void startPlay();                         // JOGAR: continua ou pega o próximo não resolvido
  int nextCaseIndex(int from) const;        // próximo caso na ordem (circular)
  int nextUnresolvedIndex(int from) const;  // -1 se todos resolvidos
  int resolvedCount() const;
  const Mystery& mystery() const { return MysteryDB::at(caseIndex); }
  uint16_t caseId() const { return mystery().id; }

  // ---- Preferências ---------------------------------------------------------
  uint16_t typeMsPerChar() const;
  FontId bodyFont() const;
  void applySettings();

  // ---- Estado compartilhado entre telas ------------------------------------
  ProgressManager progress;
  SettingsManager settings;
  AudioManager audio;
  InputManager input;
  int caseIndex = 0;
  bool introReread = false;     // apresentação aberta pelo menu de investigação
  bool clueJustRevealed = false;  // anima a dica mais recente
  int browserSelection = 0;
  ScreenId caseReturn = SCR_MENU;   // para onde ESC volta a partir de um caso

 private:
  void switchNow(uint32_t now);
  void applyTransition(M5Canvas& c, uint32_t now);
  Screen* _screens[SCR_COUNT] = {};
  Screen* _current = nullptr;
  ScreenId _currentId = SCR_SPLASH;
  ScreenId _prevId = SCR_SPLASH;
  ScreenId _pending = SCR_COUNT;
  Transition _pendingTr = TR_NONE;
  Transition _tr = TR_NONE;
  uint32_t _trStart = 0;
  uint32_t _trDur = 0;
  bool _dirty = true;
  uint32_t _lastFrame = 0;
  const char* _toastMsg = nullptr;
  uint32_t _toastUntil = 0;
  bool _dimmed = false;
};

extern GameEngine Game;
