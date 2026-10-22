#include "MysteryDatabase.h"
#include "MysteryData.h"

namespace MysteryDB {

int count() { return (int)MYSTERY_COUNT; }

const Mystery& at(int index) {
  if (index < 0) index = 0;
  if (index >= (int)MYSTERY_COUNT) index = (int)MYSTERY_COUNT - 1;
  return MYSTERIES[index];
}

int indexOfId(uint16_t id) {
  for (int i = 0; i < (int)MYSTERY_COUNT; ++i)
    if (MYSTERIES[i].id == id) return i;
  return -1;
}

const char* categoryName(uint8_t c) {
  static const char* const names[CAT_COUNT] = {"ASSASSINATOS",     "MORTES MISTERIOSAS", "CRIMES IMPOSSÍVEIS",
                                               "DESAPARECIMENTOS", "ACIDENTES FATAIS",   "SEGREDOS MACABROS",
                                               "MISTÉRIOS PSICOLÓGICOS"};
  return c < CAT_COUNT ? names[c] : "";
}

const char* difficultyName(uint8_t d) {
  static const char* const names[DIFF_COUNT] = {"FÁCIL", "MÉDIO", "DIFÍCIL", "INSANO"};
  return d < DIFF_COUNT ? names[d] : "";
}

}  // namespace MysteryDB
