#!/usr/bin/env bash
set -euo pipefail
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

# if [ -f "/usr/local/lib/libraylib.a" ] && [ -f "/usr/local/include/raylib.h" ]; then
#     prettyprint 0 "Raylib ya se encuentra instalado en el sistema. Omitiendo compilación."
#     exit 0
# fi

if [ -f "/usr/local/lib/libraylib.a" ]; then
    prettyprint 1 "Limpiando versión anterior de Raylib para asegurar soporte -fPIC. . ."
    sudo rm -f /usr/local/lib/libraylib.a
    sudo rm -f /usr/local/include/raylib.h /usr/local/include/rlgl.h /usr/local/include/raymath.h
fi

if [ ! -f "/usr/local/lib/libraylib.a" ]; then
    prettyprint 0 "Compilando e instalando Raylib (estático con -fPIC) desde la fuente oficial. . ."
    
    cd /tmp
    rm -rf raylib
    git clone https://github.com/raysan5/raylib.git
    cd raylib
    
    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
    make -j$(nproc || echo 2)
    sudo make install
    
    cd /tmp
    rm -rf raylib
fi