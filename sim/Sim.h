// Funções auxiliares do simulador de PC.
#pragma once
#include <stdint.h>
#include "../src/Platform.h"

namespace sim {
void setStorePath(const char* path);
void advance(uint32_t ms);
void setHeld(const KeySet& k);
bool savePpm(const char* path);
int presents();
int audioEvents();
extern int storageWrites;
}  // namespace sim
