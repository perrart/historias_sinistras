#include "SettingsManager.h"
#include <string.h>
#include "Platform.h"

static const char* KEY_SETTINGS = "cfg";

void SettingsManager::begin() {
  SettingsData tmp;
  if (hal::storageRead(KEY_SETTINGS, &tmp, sizeof(tmp)) && tmp.version == 1) {
    _d = tmp;
  }
  // saneamento
  if (_d.sound > 1) _d.sound = 1;
  if (_d.volume > 2) _d.volume = 1;
  if (_d.textSpeed > SPEED_INSTANT) _d.textSpeed = SPEED_NORMAL;
  if (_d.textSize > SIZE_LARGE) _d.textSize = SIZE_NORMAL;
  if (_d.brightness > 3) _d.brightness = 2;
  _saved = _d;
  _dirty = false;
}

void SettingsManager::save() {
  if (!_dirty) return;
  _dirty = false;
  if (memcmp(&_d, &_saved, sizeof(_d)) == 0) return;  // nada mudou de fato
  if (hal::storageWrite(KEY_SETTINGS, &_d, sizeof(_d))) _saved = _d;
}

void SettingsManager::resetToDefaults() {
  _d = SettingsData();
  _dirty = true;
}

uint8_t SettingsManager::brightnessLevel() const {
  static const uint8_t lv[4] = {50, 100, 170, 245};
  return lv[_d.brightness & 3];
}

uint8_t SettingsManager::masterVolume() const {
  static const uint8_t lv[3] = {70, 140, 220};
  return lv[_d.volume % 3];
}
