// Banco de mistérios embutido na flash (gerado a partir de content/casos.txt
// pelo script tools/gen_mysteries.py — para adicionar casos, edite o .txt e rode o script).
#pragma once
#include <stdint.h>

enum Category : uint8_t {
  CAT_ASSASSINATOS = 0,
  CAT_MORTES_MISTERIOSAS,
  CAT_CRIMES_IMPOSSIVEIS,
  CAT_DESAPARECIMENTOS,
  CAT_ACIDENTES_FATAIS,
  CAT_SEGREDOS_MACABROS,
  CAT_MISTERIOS_PSICOLOGICOS,
  CAT_COUNT
};

enum Difficulty : uint8_t { DIFF_FACIL = 0, DIFF_MEDIO, DIFF_DIFICIL, DIFF_INSANO, DIFF_COUNT };

struct Mystery {
  uint16_t id;
  uint8_t category;
  uint8_t difficulty;
  const char* title;
  const char* scenario;
  const char* clues[4];
  const char* solution;
};

namespace MysteryDB {

int count();
const Mystery& at(int index);
int indexOfId(uint16_t id);          // -1 se não existir
const char* categoryName(uint8_t c);
const char* difficultyName(uint8_t d);

}  // namespace MysteryDB
