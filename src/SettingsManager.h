// Configurações do jogador, salvas na NVS somente quando mudam.
#pragma once
#include <stdint.h>

enum TextSpeed : uint8_t { SPEED_NORMAL = 0, SPEED_FAST = 1, SPEED_INSTANT = 2 };
enum TextSize : uint8_t { SIZE_NORMAL = 0, SIZE_LARGE = 1 };

struct SettingsData {
  uint8_t version = 1;
  uint8_t sound = 1;        // 0/1
  uint8_t volume = 1;       // 0 baixo, 1 médio, 2 alto
  uint8_t textSpeed = SPEED_NORMAL;
  uint8_t textSize = SIZE_NORMAL;
  uint8_t brightness = 2;   // 0..3
  uint8_t reserved[2] = {0, 0};
};

class SettingsManager {
 public:
  void begin();
  SettingsData& data() { return _d; }
  const SettingsData& data() const { return _d; }
  void markDirty() { _dirty = true; }
  void save();                 // grava se houver mudança
  void resetToDefaults();
  uint8_t brightnessLevel() const;  // 0..255 para o LCD
  uint8_t masterVolume() const;     // 0..255 para o alto-falante

 private:
  SettingsData _d;
  SettingsData _saved;
  bool _dirty = false;
};
