// Converte o estado do teclado em eventos discretos, com debounce,
// detecção de borda (uma ação por toque) e auto-repetição só para as setas.
#pragma once
#include <stdint.h>
#include "Platform.h"

struct InputEvent {
  uint8_t key = 0;     // Key
  char ch = 0;         // 'a'..'z' ou '0'..'9' quando aplicável
  bool repeat = false; // gerado por auto-repetição
  bool is(uint8_t k) const { return key == k; }
  bool isChar(char c) const { return ch == c; }
  bool confirm() const { return key == K_ENTER; }
  bool back() const { return key == K_BACK; }
};

class InputManager {
 public:
  void poll(uint32_t now);
  bool next(InputEvent& ev);
  bool anyHeld() const { return _held.bits != 0; }
  uint32_t lastActivity() const { return _lastActivity; }
  void suppressUntilRelease();   // ignora teclas seguradas (ex.: após trocar de tela)

 private:
  void push(uint8_t key, bool repeat);
  KeySet _held;
  uint64_t _suppressed = 0;
  uint32_t _pressTime[K_COUNT] = {};
  uint32_t _lastRepeat[K_COUNT] = {};
  uint32_t _lastEdge[K_COUNT] = {};
  InputEvent _queue[8];
  uint8_t _head = 0, _tail = 0;
  uint32_t _lastActivity = 0;
};
