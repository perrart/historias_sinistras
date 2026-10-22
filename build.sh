#!/usr/bin/env bash
# Compila HISTÓRIAS SINISTRAS com o arduino-cli (alternativa ao PlatformIO).
# Requer: arduino-cli 1.x, git e python3. Gera os binários em release/.
#
#   ./build.sh
#
set -euo pipefail
cd "$(dirname "$0")"

CORE_VERSION="3.3.12"
FQBN="esp32:esp32:m5stack_cardputer:PartitionScheme=default_8MB,FlashSize=8M,CDCOnBoot=cdc"
WORK="${WORK:-$PWD/.arduino}"
export ARDUINO_DIRECTORIES_DATA="$WORK/data"
export ARDUINO_DIRECTORIES_USER="$WORK/user"
export ARDUINO_DIRECTORIES_DOWNLOADS="$WORK/staging"
export ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

echo "==> Validando e gerando o banco de casos"
python3 tools/gen_mysteries.py

echo "==> Instalando o núcleo ESP32 $CORE_VERSION"
arduino-cli core update-index
arduino-cli core install "esp32:esp32@$CORE_VERSION"

echo "==> Baixando bibliotecas nas versões testadas"
LIBS="$ARDUINO_DIRECTORIES_USER/libraries"
mkdir -p "$LIBS"
fetch() {  # nome url ref
  if [ ! -d "$LIBS/$1/.git" ]; then git clone -q "$2" "$LIBS/$1"; fi
  git -C "$LIBS/$1" fetch -q --tags origin
  git -C "$LIBS/$1" -c advice.detachedHead=false checkout -q "$3"
}
fetch M5GFX https://github.com/m5stack/M5GFX.git 0.2.34
fetch M5Unified https://github.com/m5stack/M5Unified.git 0.2.27
fetch M5Cardputer https://github.com/m5stack/M5Cardputer.git f1392858b9994c3547120e602a57d3553d16ab01

echo "==> Compilando"
rm -rf "$WORK/out"
arduino-cli compile -b "$FQBN" --warnings default --output-dir "$WORK/out" .

echo "==> Montando a imagem completa"
mkdir -p release
ESPTOOL=$(find "$ARDUINO_DIRECTORIES_DATA/packages/esp32/tools/esptool_py" -name esptool -type f | head -1)
BOOT_APP0=$(find "$ARDUINO_DIRECTORIES_DATA/packages/esp32/hardware/esp32" -name boot_app0.bin | head -1)
cp "$WORK/out/historias_sinistras.ino.bin" release/historias_sinistras_app.bin
"$ESPTOOL" --chip esp32s3 merge-bin -o release/historias_sinistras.bin \
  --flash-mode dio --flash-freq 80m --flash-size 8MB \
  0x0 "$WORK/out/historias_sinistras.ino.bootloader.bin" \
  0x8000 "$WORK/out/historias_sinistras.ino.partitions.bin" \
  0xe000 "$BOOT_APP0" \
  0x10000 "$WORK/out/historias_sinistras.ino.bin"

echo
echo "Pronto:"
echo "  release/historias_sinistras.bin      -> imagem completa, gravar em 0x0"
echo "  release/historias_sinistras_app.bin  -> somente o app, gravar em 0x10000"
