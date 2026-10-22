// HISTÓRIAS SINISTRAS — jogo de mistério para M5Stack Cardputer ADV.
// Ponto de entrada Arduino: toda a lógica vive no GameEngine (máquina de estados não bloqueante).
#if defined(ARDUINO)
#include <Arduino.h>
#include "GameEngine.h"

void setup() { Game.begin(); }

void loop() {
  Game.tick();
  delay(1);  // cede tempo às tarefas do sistema (Wi-Fi desligado, áudio em DMA)
}
#endif
