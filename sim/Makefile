# Simulador de PC do HISTÓRIAS SINISTRAS (Linux/macOS, requer SDL2 e a biblioteca M5GFX).
#   make -C sim M5GFX=/caminho/para/M5GFX
#   ./sim/build/hs_sim sim/out && python3 tools/ppm2png.py sim/out
M5GFX ?= ../../M5GFX
CXX ?= g++
CC ?= gcc
BUILD := build
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wno-unused-function -I../src -I$(M5GFX)/src $(shell sdl2-config --cflags 2>/dev/null)
CFLAGS := -O1 -I$(M5GFX)/src
LDLIBS := $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2) -lpthread

GFX_CPP := $(M5GFX)/src/M5GFX.cpp $(wildcard $(M5GFX)/src/lgfx/v1/*.cpp)
GFX_C := $(wildcard $(M5GFX)/src/lgfx/utility/*.c) $(wildcard $(M5GFX)/src/lgfx/Fonts/efont/*.c) $(wildcard $(M5GFX)/src/lgfx/Fonts/IPA/*.c)
GAME := $(filter-out ../src/PlatformDevice.cpp ../src/main.cpp,$(wildcard ../src/*.cpp))
SIM := PlatformSim.cpp sim_main.cpp

OBJ := $(patsubst %.cpp,$(BUILD)/gfx/%.o,$(notdir $(GFX_CPP))) \
       $(patsubst %.c,$(BUILD)/gfx/%.o,$(notdir $(GFX_C))) \
       $(patsubst %.cpp,$(BUILD)/game/%.o,$(notdir $(GAME))) \
       $(patsubst %.cpp,$(BUILD)/sim/%.o,$(SIM))

vpath %.cpp $(M5GFX)/src $(M5GFX)/src/lgfx/v1 ../src .
vpath %.c $(M5GFX)/src/lgfx/utility $(M5GFX)/src/lgfx/Fonts/efont $(M5GFX)/src/lgfx/Fonts/IPA

all: $(BUILD)/hs_sim

$(BUILD)/hs_sim: $(OBJ)
	$(CXX) -o $@ $^ $(LDLIBS)

$(BUILD)/gfx/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -w -c $< -o $@
$(BUILD)/gfx/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -w -c $< -o $@
$(BUILD)/game/%.o: ../src/%.cpp $(wildcard ../src/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BUILD)/sim/%.o: %.cpp Sim.h $(wildcard ../src/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
.PHONY: all clean
