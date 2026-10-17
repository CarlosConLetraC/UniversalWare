#!/usr/bin/env bash
set -euo pipefail
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Clonando y compilando litebrowser-linux (litehtml) con soporte GTK3. . ."

# Exportar las rutas de pkg-config para que CMake encuentre gdkmm-3.0 y gtkmm-3.0
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:/usr/local/lib/pkgconfig"

cd /tmp
rm -rf litebrowser-linux

if git clone --recursive https://github.com/litehtml/litebrowser-linux.git; then
    cd litebrowser-linux
    
    COMPAT_COMMIT=$(git rev-list -n 1 --before="2023-02-01" HEAD)
    if [ -n "$COMPAT_COMMIT" ]; then
        prettyprint 0 "Cambiando al commit compatible: $COMPAT_COMMIT"
        git checkout "$COMPAT_COMMIT"
        git submodule update --init --recursive
    else
        prettyprint 1 "No se encontró un commit anterior a la fecha límite, usando HEAD actual."
    fi

    [ -d "build" ] && rm -rf build
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc || echo 2)
    prettyprint 0 "OK: litebrowser-linux compilado con éxito en /tmp/litebrowser-linux"
else
    prettyprint 2 "Fallo al clonar el repositorio de litebrowser-linux"
    exit 1
fi