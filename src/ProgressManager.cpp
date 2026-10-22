#include "ProgressManager.h"
#include <string.h>
#include "Platform.h"

static const char* KEY_PROGRESS = "prog";
static constexpr uint8_t BLOB_VERSION = 1;
static constexpr uint8_t MASK_CLUES = 0x07, BIT_RESOLVED = 0x08, BIT_OPENED = 0x10;

void ProgressManager::begin() {
  memset(&_b, 0, sizeof(_b));
  Blob tmp;
  if (hal::storageRead(KEY_PROGRESS, &tmp, sizeof(tmp)) && tmp.version == BLOB_VERSION) {
    _b = tmp;
  }
  _b.version = BLOB_VERSION;
  for (auto& s : _b.state) {
    if ((s & MASK_CLUES) > 4) s = (uint8_t)((s & ~MASK_CLUES) | 4);
  }
  _dirty = false;
}

void ProgressManager::save() {
  if (!_dirty) return;
  _dirty = false;
  hal::storageWrite(KEY_PROGRESS, &_b, sizeof(_b));
}

uint8_t& ProgressManager::st(uint16_t id) {
  static uint8_t dummy = 0;
  if (id > cfg::MAX_CASE_ID) { dummy = 0; return dummy; }
  return _b.state[id];
}

uint8_t ProgressManager::stc(uint16_t id) const {
  return id > cfg::MAX_CASE_ID ? 0 : _b.state[id];
}

uint8_t ProgressManager::clues(uint16_t id) const { return stc(id) & MASK_CLUES; }

void ProgressManager::setClues(uint16_t id, uint8_t n) {
  if (n > 4) n = 4;
  uint8_t& s = st(id);
  const uint8_t v = (uint8_t)((s & ~MASK_CLUES) | n | BIT_OPENED);
  if (v != s) { s = v; _dirty = true; }
}

bool ProgressManager::resolved(uint16_t id) const { return stc(id) & BIT_RESOLVED; }

void ProgressManager::markResolved(uint16_t id) {
  uint8_t& s = st(id);
  const uint8_t v = (uint8_t)(s | BIT_RESOLVED | BIT_OPENED);
  if (v != s) { s = v; _dirty = true; }
}

void ProgressManager::setResolved(uint16_t id, bool on) {
  if (on) { markResolved(id); return; }
  uint8_t& s = st(id);
  const uint8_t v = (uint8_t)((s & ~BIT_RESOLVED) | BIT_OPENED);
  if (v != s) { s = v; _dirty = true; }
}

void ProgressManager::markOpened(uint16_t id) {
  uint8_t& s = st(id);
  if (!(s & BIT_OPENED)) { s |= BIT_OPENED; _dirty = true; }
}

CaseStatus ProgressManager::status(uint16_t id) const {
  const uint8_t s = stc(id);
  if (s & BIT_RESOLVED) return STATUS_RESOLVED;
  if (s & BIT_OPENED) return STATUS_IN_PROGRESS;
  return STATUS_NEW;
}

void ProgressManager::restartCase(uint16_t id) {
  uint8_t& s = st(id);
  const uint8_t v = (uint8_t)(s & ~MASK_CLUES);
  if (v != s) { s = v; _dirty = true; }
}

void ProgressManager::setCurrentCase(uint16_t id) {
  if (_b.current != id) { _b.current = id; _dirty = true; }
}

void ProgressManager::resetAll() {
  memset(&_b, 0, sizeof(_b));
  _b.version = BLOB_VERSION;
  _dirty = true;
}
