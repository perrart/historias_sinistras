#include "GameEngine.h"
#include "Config.h"
#include "Platform.h"
#include "ScreenManager.h"
#include "Screens.h"

GameEngine Game;

void GameEngine::begin() {
  hal::begin();
  Fonts::begin();
  settings.begin();
  progress.begin();
  audio.begin();
  applySettings();

  for (int i = 0; i < SCR_COUNT; ++i) _screens[i] = screens::get((ScreenId)i);

  // retoma o último caso aberto
  const int idx = MysteryDB::indexOfId(progress.currentCase());
  caseIndex = idx >= 0 ? idx : 0;
  browserSelection = caseIndex;

  _currentId = SCR_SPLASH;
  _current = _screens[SCR_SPLASH];
  _current->enter();
  _dirty = true;
}

void GameEngine::applySettings() {
  const auto& s = settings.data();
  audio.setEnabled(s.sound != 0);
  audio.setVolume(settings.masterVolume());
  if (!_dimmed) hal::setBrightness(settings.brightnessLevel());
}

uint16_t GameEngine::typeMsPerChar() const {
  switch (settings.data().textSpeed) {
    case SPEED_FAST: return 5;
    case SPEED_INSTANT: return 0;
    default: return 16;
  }
}

FontId GameEngine::bodyFont() const { return settings.data().textSize == SIZE_LARGE ? F_BODY_L : F_BODY; }

void GameEngine::go(ScreenId id, Transition t) {
  _pending = id;
  _pendingTr = t;
}

void GameEngine::toast(const char* msg, uint32_t ms) {
  _toastMsg = msg;
  _toastUntil = hal::now() + ms;
  _dirty = true;
}

void GameEngine::switchNow(uint32_t now) {
  const ScreenId id = _pending;
  _pending = SCR_COUNT;
  if (_current) _current->leave();
  _prevId = _currentId;
  _currentId = id;
  _current = _screens[id];
  input.suppressUntilRelease();
  _current->enter();
  _tr = _pendingTr;
  _trStart = now;
  switch (_tr) {
    case TR_GLITCH: _trDur = 230; break;
    case TR_WIPE: _trDur = 280; break;
    case TR_FLASH: _trDur = 420; break;
    default: _trDur = 0; break;
  }
  _dirty = true;
}

void GameEngine::applyTransition(M5Canvas& c, uint32_t now) {
  if (_tr == TR_NONE) return;
  const uint32_t el = now - _trStart;
  if (el >= _trDur) {
    _tr = TR_NONE;
    return;
  }
  const float t = (float)el / (float)_trDur;
  switch (_tr) {
    case TR_GLITCH:
      ui::glitch(c, now * 2654435761u, 1.0f - t);
      break;
    case TR_WIPE:
      ui::wipe(c, t);
      break;
    case TR_FLASH:
      c.fillRectAlpha(0, 0, cfg::SCREEN_W, cfg::SCREEN_H, (uint8_t)((1.0f - t) * 200), cfg::C_RED);
      if (t < 0.5f) ui::glitch(c, now * 2246822519u, 0.8f - t);
      break;
    default:
      break;
  }
}

void GameEngine::tick() {
  const uint32_t now = hal::now();
  input.poll(now);

  InputEvent ev;
  while (_pending == SCR_COUNT && input.next(ev)) {
    if (_dimmed) {  // primeira tecla só acorda a tela
      _dimmed = false;
      hal::setBrightness(settings.brightnessLevel());
      continue;
    }
    _current->input(ev);
  }

  // economia de tela por inatividade
  if (!_dimmed && input.lastActivity() && now - input.lastActivity() > cfg::DIM_AFTER_MS) {
    _dimmed = true;
    hal::setBrightness(12);
  }

  if (_pending != SCR_COUNT) switchNow(now);

  bool need = _dirty;
  if (_current->update(now)) need = true;
  if (_tr != TR_NONE) need = true;
  if (_toastMsg && (int32_t)(now - _toastUntil) >= 0) {
    _toastMsg = nullptr;
    need = true;
  }

  if (need) {
    if (now - _lastFrame >= cfg::FRAME_MS) {
      M5Canvas& c = hal::canvas();
      _current->draw(c);
      if (_toastMsg) ui::toast(c, _toastMsg);
      applyTransition(c, now);
      hal::present();
      _lastFrame = now;
      _dirty = false;
    } else {
      _dirty = true;
    }
  }

  audio.update(now);
  progress.save();
}

// ---------------------------------------------------------------------------
// Fluxo de casos
// ---------------------------------------------------------------------------
int GameEngine::resolvedCount() const {
  int n = 0;
  for (int i = 0; i < MysteryDB::count(); ++i)
    if (progress.resolved(MysteryDB::at(i).id)) ++n;
  return n;
}

int GameEngine::nextCaseIndex(int from) const {
  const int n = MysteryDB::count();
  return n ? (from + 1) % n : 0;
}

int GameEngine::nextUnresolvedIndex(int from) const {
  const int n = MysteryDB::count();
  for (int k = 1; k <= n; ++k) {
    const int i = (from + k) % n;
    if (!progress.resolved(MysteryDB::at(i).id)) return i;
  }
  return -1;
}

void GameEngine::openCase(int index) {
  caseIndex = index;
  browserSelection = index;
  const uint16_t id = mystery().id;
  progress.setCurrentCase(id);
  introReread = false;
  clueJustRevealed = false;
  audio.play(SFX_OPEN_CASE);
  go(SCR_INTRO, TR_WIPE);
}

void GameEngine::startPlay() {
  const int cur = MysteryDB::indexOfId(progress.currentCase());
  if (cur >= 0 && progress.status(MysteryDB::at(cur).id) == STATUS_IN_PROGRESS) {
    openCase(cur);
    return;
  }
  // primeiro caso nunca aberto, ou o próximo não resolvido
  const int start = cur >= 0 ? cur : -1;
  const int nxt = nextUnresolvedIndex(start < 0 ? MysteryDB::count() - 1 : start);
  if (nxt < 0) {
    toast("TODOS OS CASOS FORAM RESOLVIDOS!", 2200);
    go(SCR_BROWSER, TR_GLITCH);
    return;
  }
  openCase(nxt);
}
