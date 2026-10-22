// HAL do simulador de PC: framebuffer em memória, relógio virtual, teclado
// roteirizado e "NVS" num arquivo. Usado para capturas de tela e testes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

#include "../src/Config.h"
#include "../src/Platform.h"
#include "Sim.h"

namespace {
M5Canvas* g_canvas = nullptr;
uint32_t g_now = 0;
KeySet g_held;
uint32_t g_rng = 12345;
std::map<std::string, std::vector<uint8_t>> g_store;
std::string g_storePath;
int g_presents = 0;
int g_audioEvents = 0;

void loadStore() {
  g_store.clear();
  FILE* f = fopen(g_storePath.c_str(), "rb");
  if (!f) return;
  char key[64];
  uint32_t len;
  while (fread(key, 1, sizeof(key), f) == sizeof(key) && fread(&len, 4, 1, f) == 1) {
    std::vector<uint8_t> v(len);
    if (fread(v.data(), 1, len, f) != len) break;
    g_store[key] = v;
  }
  fclose(f);
}

void saveStore() {
  FILE* f = fopen(g_storePath.c_str(), "wb");
  if (!f) return;
  for (auto& kv : g_store) {
    char key[64] = {0};
    strncpy(key, kv.first.c_str(), 63);
    uint32_t len = (uint32_t)kv.second.size();
    fwrite(key, 1, sizeof(key), f);
    fwrite(&len, 4, 1, f);
    fwrite(kv.second.data(), 1, len, f);
  }
  fclose(f);
}
}  // namespace

namespace sim {
void setStorePath(const char* p) {
  g_storePath = p;
  loadStore();
}
void advance(uint32_t ms) { g_now += ms; }
void setHeld(const KeySet& k) { g_held = k; }
int presents() { return g_presents; }
int audioEvents() { return g_audioEvents; }
int storageWrites = 0;

bool savePpm(const char* path) {
  M5Canvas& c = *g_canvas;
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  fprintf(f, "P6\n%d %d\n255\n", c.width(), c.height());
  const uint16_t* b = (const uint16_t*)c.getBuffer();
  for (int i = 0; i < c.width() * c.height(); ++i) {
    uint16_t v = (uint16_t)((b[i] >> 8) | (b[i] << 8));
    uint8_t rgb[3] = {(uint8_t)(((v >> 11) & 0x1F) * 255 / 31), (uint8_t)(((v >> 5) & 0x3F) * 255 / 63),
                      (uint8_t)((v & 0x1F) * 255 / 31)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
  return true;
}
}  // namespace sim

namespace hal {

void begin() {
  g_canvas = new M5Canvas();
  g_canvas->setColorDepth(16);
  g_canvas->createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
  g_canvas->setTextWrap(false, false);
}

uint32_t now() { return g_now; }
uint32_t randomU32() {
  g_rng = g_rng * 1103515245u + 12345u;
  return g_rng;
}
M5Canvas& canvas() { return *g_canvas; }
void present() { ++g_presents; }
void setBrightness(uint8_t) {}
void readKeys(KeySet& out) { out = g_held; }

bool audioOk() { return true; }
void audioMaster(uint8_t) {}
void audioTone(float, uint32_t, uint8_t, uint8_t) { ++g_audioEvents; }
void audioRaw(const int8_t*, size_t, uint32_t, uint8_t, uint8_t) { ++g_audioEvents; }
void audioStopAll() {}

bool storageRead(const char* key, void* buf, size_t len) {
  auto it = g_store.find(key);
  if (it == g_store.end() || it->second.size() != len) return false;
  memcpy(buf, it->second.data(), len);
  return true;
}
bool storageWrite(const char* key, const void* buf, size_t len) {
  g_store[key] = std::vector<uint8_t>((const uint8_t*)buf, (const uint8_t*)buf + len);
  ++sim::storageWrites;
  if (!g_storePath.empty()) saveStore();
  return true;
}
void storageErase() {
  g_store.clear();
  if (!g_storePath.empty()) saveStore();
}
int batteryLevel() { return 72; }
bool charging() { return false; }

}  // namespace hal
