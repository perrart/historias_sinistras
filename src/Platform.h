// Camada de abstração de hardware (HAL).
// O jogo só conversa com o hardware por aqui; assim o mesmo código roda no
// Cardputer ADV (PlatformDevice.cpp) e no simulador de PC (sim/PlatformSim.cpp),
// usado para gerar capturas de tela e testar a navegação.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <M5GFX.h>

// Teclas lógicas (bits de KeySet)
enum Key : uint8_t {
  K_A = 0,            // 0..25 = 'a'..'z'
  K_0 = 26,           // 26..35 = '0'..'9'
  K_UP = 36,
  K_DOWN,
  K_LEFT,
  K_RIGHT,
  K_ENTER,
  K_BACK,             // ESC (tecla ` ) ou BACKSPACE
  K_SPACE,
  K_TAB,
  K_COUNT
};

struct KeySet {
  uint64_t bits = 0;
  bool has(uint8_t k) const { return (bits >> k) & 1ULL; }
  void set(uint8_t k) { bits |= (1ULL << k); }
};

namespace hal {

void begin();
uint32_t now();
uint32_t randomU32();

// --- Vídeo -----------------------------------------------------------------
M5Canvas& canvas();          // framebuffer 240x135 (RGB565)
void present();              // envia o framebuffer para o LCD
void setBrightness(uint8_t level);

// --- Entrada ---------------------------------------------------------------
void readKeys(KeySet& out);  // teclas atualmente pressionadas

// --- Áudio (não bloqueante) ------------------------------------------------
bool audioOk();
void audioMaster(uint8_t volume);
void audioTone(float freqHz, uint32_t ms, uint8_t channel, uint8_t volume);
void audioRaw(const int8_t* data, size_t len, uint32_t sampleRate, uint8_t channel, uint8_t volume);
void audioStopAll();

// --- Armazenamento persistente (NVS) ---------------------------------------
bool storageRead(const char* key, void* buf, size_t len);
bool storageWrite(const char* key, const void* buf, size_t len);
void storageErase();

// --- Energia ---------------------------------------------------------------
int batteryLevel();          // 0..100, ou -1 se desconhecido
bool charging();

}  // namespace hal
