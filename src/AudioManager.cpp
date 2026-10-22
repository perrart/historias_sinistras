#include "AudioManager.h"
#include <math.h>
#include "Platform.h"

namespace {

enum Wave : uint8_t { W_TONE = 0, W_CLICK, W_THUMP, W_STATIC };

// Amostras geradas uma única vez na inicialização (RAM estática, sem alocação).
int8_t s_click[220];
int8_t s_thump[2600];
int8_t s_static[2400];

struct Note { uint16_t at, freq, dur; uint8_t vol, wave; };
struct Effect { const Note* notes; uint8_t count; };

#define N(at, f, d, v, w) {at, f, d, v, w}
const Note FX_NAV[]   = {N(0, 0, 0, 150, W_CLICK)};
const Note FX_SEL[]   = {N(0, 660, 35, 110, W_TONE), N(40, 988, 70, 110, W_TONE)};
const Note FX_BACK[]  = {N(0, 523, 35, 100, W_TONE), N(40, 330, 80, 100, W_TONE)};
const Note FX_OPEN[]  = {N(0, 0, 0, 190, W_THUMP), N(0, 0, 0, 70, W_STATIC), N(60, 196, 420, 70, W_TONE),
                         N(60, 207, 420, 55, W_TONE)};
const Note FX_CLUE[]  = {N(0, 0, 0, 120, W_CLICK), N(10, 392, 110, 110, W_TONE), N(130, 370, 110, 110, W_TONE),
                         N(260, 294, 420, 120, W_TONE), N(260, 277, 420, 80, W_TONE)};
const Note FX_HEART[] = {N(0, 0, 0, 230, W_THUMP), N(190, 0, 0, 170, W_THUMP), N(950, 0, 0, 230, W_THUMP),
                         N(1140, 0, 0, 170, W_THUMP)};
const Note FX_REVEAL[] = {N(0, 0, 0, 120, W_STATIC), N(0, 0, 0, 240, W_THUMP), N(140, 220, 1100, 110, W_TONE),
                          N(140, 233, 1100, 90, W_TONE), N(140, 311, 1100, 80, W_TONE), N(700, 110, 900, 120, W_TONE)};
const Note FX_SOLVED[] = {N(0, 0, 0, 230, W_THUMP), N(0, 0, 0, 200, W_CLICK), N(180, 440, 120, 110, W_TONE),
                          N(310, 523, 120, 110, W_TONE), N(440, 659, 160, 110, W_TONE), N(620, 220, 600, 120, W_TONE)};
const Note FX_TYPE[]  = {N(0, 0, 0, 70, W_CLICK)};
const Note FX_DENY[]  = {N(0, 147, 140, 120, W_TONE), N(0, 156, 140, 90, W_TONE)};
const Note FX_BOOT[]  = {N(0, 0, 0, 90, W_STATIC), N(200, 0, 0, 220, W_THUMP), N(260, 110, 1400, 90, W_TONE),
                         N(260, 117, 1400, 70, W_TONE), N(1200, 0, 0, 200, W_THUMP)};
const Note FX_GLITCH[] = {N(0, 0, 0, 110, W_STATIC)};
#undef N

#define FX(a) {a, (uint8_t)(sizeof(a) / sizeof(a[0]))}
const Effect EFFECTS[SFX_COUNT] = {FX(FX_NAV),    FX(FX_SEL),   FX(FX_BACK),   FX(FX_OPEN),
                                   FX(FX_CLUE),   FX(FX_HEART), FX(FX_REVEAL), FX(FX_SOLVED),
                                   FX(FX_TYPE),   FX(FX_DENY),  FX(FX_BOOT),   FX(FX_GLITCH)};
#undef FX

void generateSamples() {
  uint32_t seed = 0x1234567u;
  auto rnd = [&seed]() {
    seed = seed * 1664525u + 1013904223u;
    return (int)((seed >> 24) & 0xFF) - 128;
  };
  // clique seco de máquina de escrever: ruído com decaimento rápido
  for (int i = 0; i < (int)sizeof(s_click); ++i) {
    float env = expf(-i / 28.0f);
    s_click[i] = (int8_t)(rnd() * env * 0.9f);
  }
  // batida grave (coração/porta): seno descendente com leve saturação
  float phase = 0;
  for (int i = 0; i < (int)sizeof(s_thump); ++i) {
    float t = i / 16000.0f;
    float f = 46.0f + 90.0f * expf(-t * 28.0f);
    phase += 2.0f * 3.14159265f * f / 16000.0f;
    float env = expf(-t * 16.0f) * (i < 40 ? i / 40.0f : 1.0f);
    float v = sinf(phase) * 1.6f;
    v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
    s_thump[i] = (int8_t)(v * env * 120.0f);
  }
  // estática de rádio: ruído filtrado com envelope de entrada/saída
  float lp = 0;
  for (int i = 0; i < (int)sizeof(s_static); ++i) {
    lp += (rnd() - lp) * 0.45f;
    float x = (float)i / sizeof(s_static);
    float env = sinf(x * 3.14159265f);
    float crackle = ((rnd() & 0x1F) == 0) ? 1.8f : 1.0f;
    float v = lp * env * crackle * 0.9f;
    s_static[i] = (int8_t)(v > 127 ? 127 : (v < -127 ? -127 : v));
  }
}

}  // namespace

void AudioManager::begin() {
  generateSamples();
  _ok = hal::audioOk();
}

void AudioManager::setEnabled(bool on) {
  _enabled = on;
  if (!on) stopAll();
}

void AudioManager::setVolume(uint8_t master) { hal::audioMaster(master); }

void AudioManager::stopAll() {
  for (auto& p : _q) p.used = false;
  hal::audioStopAll();
}

void AudioManager::play(Sfx s) {
  if (!_enabled || !_ok || s >= SFX_COUNT) return;
  const uint32_t now = hal::now();
  if (s == SFX_TYPE) {               // limita o tique de digitação
    if (now - _lastType < 65) return;
    _lastType = now;
  }
  const Effect& e = EFFECTS[s];
  for (uint8_t i = 0; i < e.count; ++i) {
    for (auto& p : _q) {
      if (p.used) continue;
      const Note& n = e.notes[i];
      p.used = true;
      p.at = now + n.at;
      p.freq = n.freq;
      p.dur = n.dur;
      p.vol = n.vol;
      p.wave = n.wave;
      // canal 7 reservado ao tique de digitação; demais em rodízio 0..6
      if (s == SFX_TYPE || s == SFX_NAV) p.channel = 7;
      else { p.channel = _nextCh; _nextCh = (uint8_t)((_nextCh + 1) % 7); }
      break;
    }
  }
  update(now);
}

void AudioManager::update(uint32_t now) {
  if (!_enabled || !_ok) return;
  for (auto& p : _q) {
    if (!p.used || (int32_t)(now - p.at) < 0) continue;
    p.used = false;
    switch (p.wave) {
      case W_TONE:   hal::audioTone(p.freq, p.dur, p.channel, p.vol); break;
      case W_CLICK:  hal::audioRaw(s_click, sizeof(s_click), 22050, p.channel, p.vol); break;
      case W_THUMP:  hal::audioRaw(s_thump, sizeof(s_thump), 16000, p.channel, p.vol); break;
      case W_STATIC: hal::audioRaw(s_static, sizeof(s_static), 16000, p.channel, p.vol); break;
    }
  }
}
