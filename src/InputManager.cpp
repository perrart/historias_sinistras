#include "InputManager.h"
#include "Config.h"

static bool isRepeatable(uint8_t k) { return k == K_UP || k == K_DOWN || k == K_LEFT || k == K_RIGHT; }

void InputManager::push(uint8_t key, bool repeat) {
  const uint8_t next = (uint8_t)((_tail + 1) % 8);
  if (next == _head) return;  // fila cheia: descarta
  InputEvent& e = _queue[_tail];
  e.key = key;
  e.repeat = repeat;
  e.ch = 0;
  if (key < 26) e.ch = (char)('a' + key);
  else if (key >= K_0 && key < K_0 + 10) e.ch = (char)('0' + (key - K_0));
  _tail = next;
}

void InputManager::poll(uint32_t now) {
  KeySet cur;
  hal::readKeys(cur);

  // libera teclas suprimidas assim que forem soltas
  _suppressed &= cur.bits;

  for (uint8_t k = 0; k < K_COUNT; ++k) {
    const bool was = _held.has(k);
    const bool is = cur.has(k);
    if (is && !was) {
      _pressTime[k] = now;
      _lastRepeat[k] = now;
      if (now - _lastEdge[k] >= cfg::DEBOUNCE_MS && !((_suppressed >> k) & 1ULL)) {
        _lastEdge[k] = now;
        push(k, false);
      }
      _lastActivity = now;
    } else if (is && was && isRepeatable(k) && !((_suppressed >> k) & 1ULL)) {
      if (now - _pressTime[k] >= cfg::REPEAT_DELAY && now - _lastRepeat[k] >= cfg::REPEAT_RATE) {
        _lastRepeat[k] = now;
        push(k, true);
        _lastActivity = now;
      }
    }
  }
  _held = cur;
}

bool InputManager::next(InputEvent& ev) {
  if (_head == _tail) return false;
  ev = _queue[_head];
  _head = (uint8_t)((_head + 1) % 8);
  return true;
}

void InputManager::suppressUntilRelease() {
  _suppressed = _held.bits;
  _head = _tail = 0;
}
