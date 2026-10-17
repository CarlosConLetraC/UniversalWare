#!/usr/bin/env bash
set -euo pipefail
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Verificando versión de Meson en la etapa _0..."

# Función para comparar versiones numéricas (retorna 0 si $1 < $2)
version_lt() {
    [ "$1" = "$(printf '%s\n' "$1" "$2" | sort -V | head -n1)" ] && [ "$1" != "$2" ]
}

# Verificar si meson está instalado y obtener su versión
if command -v meson &>/dev/null; then
    CURRENT_MESON_VERSION=$(meson --version)
    prettyprint 0 "Versión de Meson detectada: $CURRENT_MESON_VERSION"
    
    if version_lt "$CURRENT_MESON_VERSION" "0.62.0"; then
        prettyprint 1 "La versión de Meson es inferior a 0.62.0. Actualizando mediante pip3. . ."
        sudo apt-get install -y python3-pip python3-setuptools python3-wheel
        sudo pip3 install --upgrade meson
    else
        prettyprint 0 "La versión de Meson cumple con el requisito (>= 0.62.0)."
    fi
else
    prettyprint 0 "Meson no está instalado. Instalando la versión más reciente vía pip3. . ."
    sudo apt-get install -y python3-pip python3-setuptools python3-wheel
    sudo pip3 install --upgrade meson
fi