// Efeitos sonoros não bloqueantes. Cada efeito é uma pequena sequência de notas
// agendadas; update() dispara as notas no momento certo sem travar o jogo.
#pragma once
#include <stdint.h>

enum Sfx : uint8_t {
  SFX_NAV = 0,     // clique de navegação
  SFX_SELECT,      // confirmar
  SFX_BACK,        // voltar
  SFX_OPEN_CASE,   // abrir um caso
  SFX_CLUE,        // revelar dica
  SFX_HEARTBEAT,   // suspense (confirmação da verdade)
  SFX_REVEAL,      // a verdade
  SFX_SOLVED,      // caso concluído (carimbo)
  SFX_TYPE,        // tecla de máquina de escrever
  SFX_DENY,        // ação indisponível
  SFX_BOOT,        // abertura
  SFX_GLITCH,      // chiado de interferência
  SFX_COUNT
};

class AudioManager {
 public:
  void begin();
  void setEnabled(bool on);
  void setVolume(uint8_t master);
  void play(Sfx s);
  void update(uint32_t now);
  void stopAll();
  bool enabled() const { return _enabled; }

 private:
  struct Pending {
    uint32_t at;
    uint16_t freq;   // 0 = amostra (wave)
    uint16_t dur;
    uint8_t vol;
    uint8_t wave;
    uint8_t channel;
    bool used;
  };
  Pending _q[24] = {};
  bool _enabled = true;
  bool _ok = false;
  uint8_t _nextCh = 0;
  uint32_t _lastType = 0;
};
