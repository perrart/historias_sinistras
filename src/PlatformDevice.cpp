// Implementação da HAL para o M5Stack Cardputer ADV (também funciona no Cardputer original).
// - Vídeo: ST7789V2 via M5GFX, com framebuffer em RAM (sem cintilação).
// - Teclado: M5Cardputer detecta a placa; no ADV usa o controlador TCA8418 (I2C + INT no GPIO11).
// - Áudio: M5Unified inicializa o codec ES8311 + amplificador NS4150B do ADV (I2S1).
// - Persistência: NVS (Preferences).
#if defined(ARDUINO)

#include <Arduino.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <esp_random.h>

#include "Platform.h"
#include "Config.h"

namespace {
M5Canvas* g_canvas = nullptr;
Preferences g_prefs;
bool g_prefsOk = false;
bool g_audioOk = false;
uint8_t g_lastBrightness = 255;
}  // namespace

namespace hal {

void begin() {
  auto mcfg = M5.config();
  mcfg.serial_baudrate = 115200;
  mcfg.internal_spk = true;
  mcfg.internal_mic = false;   // o microfone compartilha o barramento I2S; não usamos
  mcfg.output_power = false;
  M5Cardputer.begin(mcfg, true);

  auto& d = M5Cardputer.Display;
  d.setRotation(1);            // paisagem 240x135
  d.setColorDepth(16);
  d.fillScreen(TFT_BLACK);

  g_canvas = new M5Canvas(&M5Cardputer.Display);
  g_canvas->setColorDepth(16);
  g_canvas->setPsram(false);
  if (!g_canvas->createSprite(cfg::SCREEN_W, cfg::SCREEN_H)) {
    // Sem RAM para o framebuffer: tenta novamente com menos profundidade
    g_canvas->setColorDepth(8);
    g_canvas->createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
  }
  g_canvas->setTextWrap(false, false);

  g_audioOk = M5Cardputer.Speaker.isEnabled();
  if (g_audioOk) {
    M5Cardputer.Speaker.setVolume(140);
  }

  g_prefsOk = g_prefs.begin("historias", false);
  Serial.printf("[HS] board=%d audio=%d nvs=%d heap=%u\n", (int)M5.getBoard(), g_audioOk, g_prefsOk,
                (unsigned)ESP.getFreeHeap());
}

uint32_t now() { return millis(); }
uint32_t randomU32() { return esp_random(); }

M5Canvas& canvas() { return *g_canvas; }

void present() { g_canvas->pushSprite(0, 0); }

void setBrightness(uint8_t level) {
  if (level == g_lastBrightness) return;
  g_lastBrightness = level;
  M5Cardputer.Display.setBrightness(level);
}

void readKeys(KeySet& out) {
  M5.update();
  // O leitor TCA8418 processa um evento por chamada; drena a fila.
  for (int i = 0; i < 6; ++i) M5Cardputer.Keyboard.updateKeyList();

  out.bits = 0;
  const auto& list = M5Cardputer.Keyboard.keyList();
  for (const auto& p : list) {
    if (p.x < 0 || p.y < 0 || p.y > 3 || p.x > 13) continue;
    const uint8_t c = (uint8_t)M5Cardputer.Keyboard.getKeyValue(p).value_first;
    switch (c) {
      case ';': out.set(K_UP); break;       // ↑ (com ou sem Fn)
      case '.': out.set(K_DOWN); break;     // ↓
      case ',': out.set(K_LEFT); break;     // ←
      case '/': out.set(K_RIGHT); break;    // →
      case '`': out.set(K_BACK); break;     // ESC (Fn + `)
      case KEY_BACKSPACE: out.set(K_BACK); break;
      case KEY_ENTER: out.set(K_ENTER); break;
      case KEY_TAB: out.set(K_TAB); break;
      case ' ': out.set(K_SPACE); break;
      default:
        if (c >= 'a' && c <= 'z') out.set(K_A + (c - 'a'));
        else if (c >= '0' && c <= '9') out.set(K_0 + (c - '0'));
        break;
    }
  }
  // Botão G0 lateral funciona como ENTER
  if (M5.BtnA.isPressed()) out.set(K_ENTER);
}

bool audioOk() { return g_audioOk; }

void audioMaster(uint8_t volume) {
  if (g_audioOk) M5Cardputer.Speaker.setVolume(volume);
}

// Onda senoidal suave (32 amostras) para tons menos estridentes que a onda padrão
static const uint8_t kSine[32] = {128, 152, 176, 198, 218, 234, 245, 253, 255, 253, 245, 234, 218, 198, 176, 152,
                                  128, 104, 80,  58,  38,  22,  11,  3,   1,   3,   11,  22,  38,  58,  80,  104};

void audioTone(float freqHz, uint32_t ms, uint8_t channel, uint8_t volume) {
  if (!g_audioOk) return;
  M5Cardputer.Speaker.setChannelVolume(channel, volume);
  M5Cardputer.Speaker.tone(freqHz, ms, channel, true, kSine, sizeof(kSine), false);
}

void audioRaw(const int8_t* data, size_t len, uint32_t sampleRate, uint8_t channel, uint8_t volume) {
  if (!g_audioOk) return;
  M5Cardputer.Speaker.setChannelVolume(channel, volume);
  M5Cardputer.Speaker.playRaw(data, len, sampleRate, false, 1, channel, true);
}

void audioStopAll() {
  if (g_audioOk) M5Cardputer.Speaker.stop();
}

bool storageRead(const char* key, void* buf, size_t len) {
  if (!g_prefsOk) return false;
  if (g_prefs.getBytesLength(key) != len) return false;
  return g_prefs.getBytes(key, buf, len) == len;
}

bool storageWrite(const char* key, const void* buf, size_t len) {
  if (!g_prefsOk) return false;
  return g_prefs.putBytes(key, buf, len) == len;
}

void storageErase() {
  if (g_prefsOk) g_prefs.clear();
}

// Leituras de bateria em cache (a medição é lenta e quase não muda)
static int s_batt = -2;
static bool s_chg = false;
static uint32_t s_battAt = 0;

static void refreshBattery() {
  const uint32_t t = millis();
  if (s_batt != -2 && t - s_battAt < 10000) return;
  s_battAt = t;
  int v = M5.Power.getBatteryLevel();
  s_batt = (v < 0 || v > 100) ? -1 : v;
  s_chg = M5.Power.isCharging() == m5::Power_Class::is_charging;
}

int batteryLevel() {
  refreshBattery();
  return s_batt;
}

bool charging() {
  refreshBattery();
  return s_chg;
}

}  // namespace hal

#endif  // ARDUINO
