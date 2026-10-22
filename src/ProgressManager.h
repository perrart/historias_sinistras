// Progresso do jogador por ID de caso (estável mesmo se novos casos forem adicionados).
// Um byte por caso: bits 0-2 = dicas reveladas (0..4), bit 3 = resolvido, bit 4 = aberto.
#pragma once
#include <stdint.h>
#include "Config.h"

enum CaseStatus : uint8_t { STATUS_NEW = 0, STATUS_IN_PROGRESS = 1, STATUS_RESOLVED = 2 };

class ProgressManager {
 public:
  void begin();
  void save();                      // grava apenas se algo mudou

  uint8_t clues(uint16_t id) const;
  void setClues(uint16_t id, uint8_t n);
  bool resolved(uint16_t id) const;
  void markResolved(uint16_t id);
  void setResolved(uint16_t id, bool on);  // marcar/desmarcar pelo menu do caso
  void markOpened(uint16_t id);
  CaseStatus status(uint16_t id) const;
  void restartCase(uint16_t id);    // zera dicas (mantém "resolvido")

  uint16_t currentCase() const { return _b.current; }
  void setCurrentCase(uint16_t id);

  void resetAll();

 private:
  struct Blob {
    uint8_t version;
    uint8_t pad;
    uint16_t current;
    uint8_t state[cfg::MAX_CASE_ID + 1];
  };
  uint8_t& st(uint16_t id);
  uint8_t stc(uint16_t id) const;
  Blob _b{};
  bool _dirty = false;
};
