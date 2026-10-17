#!/usr/bin/env bash
set -euo pipefail

BASE_PATH="$PWD"

# 1. Cargar utilidades de logs compartidas
if [ ! -f "$BASE_PATH/os/common.sh" ]; then
    printf "\e[0;31m[FAIL]:\e[0m No se encuentra os/common.sh\n"
    exit 1
fi
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Iniciando entorno modular UniversalWare (Staged Bootstrapping). . ."

export MAKEFLAGS="-j$(nproc || echo 2)"
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:/usr/local/lib/pkgconfig"

source /etc/os-release
OS=$ID
VERSION_ID="${VERSION_ID:-}"
CODENAME="${VERSION_CODENAME:-}"

if [ -z "$CODENAME" ] || [ -z "$VERSION_ID" ]; then
	prettyprint 2 "VERSION_ID o VERSION_CODENAME vacío."
	exit 1
fi

prettyprint 0 "OS detectado: $OS $VERSION_ID ($CODENAME)"

# Actualizar repositorios del sistema una sola vez al inicio
sudo apt-get update

# 2. Bootstrapping por etapas (_0, _1, _2, ...) para dependencies/ e install/
VERSION_DEP_DIR="$BASE_PATH/dependencies/$OS/$VERSION_ID"
VERSION_INSTALL_DIR="$BASE_PATH/install/$OS/$VERSION_ID"

if [ ! -d "$VERSION_DEP_DIR" ]; then
    prettyprint 2 "El directorio de dependencias por etapas no existe: $VERSION_DEP_DIR"
    exit 1
fi

# Recorrer ordenadamente cada subdirectorio de etapa numérico
for stage_dir in "$VERSION_DEP_DIR"/_[0-9]*/; do
    [ -d "$stage_dir" ] || continue
    stage_name=$(basename "$stage_dir")
    
    prettyprint 0 "=================================================="
    prettyprint 0 "Procesando Etapa de Bootstrapping: [$stage_name]"
    prettyprint 0 "=================================================="

    # A. Instalar paquetes APT definidos en los archivos .conf de esta etapa
    stage_dependencies=()
    for conf_file in "$stage_dir"/*.conf; do
        [ -f "$conf_file" ] || continue
        while IFS= read -r line || [ -n "$line" ]; do
            line=$(echo "$line" | sed 's/#.*//' | xargs)
            [ -n "$line" ] && stage_dependencies+=("$line")
        done < "$conf_file"
    done

    if [ ${#stage_dependencies[@]} -gt 0 ]; then
        prettyprint 0 "Instalando paquetes APT para la etapa $stage_name..."
        sudo apt-get install -y --no-install-recommends "${stage_dependencies[@]}"
    fi

    # B. Ejecutar scripts de instalación correspondientes a esta misma etapa
    install_stage_dir="$VERSION_INSTALL_DIR/$stage_name"
    if [ -d "$install_stage_dir" ]; then
        for install_script in "$install_stage_dir"/*.sh; do
            [ -f "$install_script" ] || continue
            script_name=$(basename "$install_script")
            prettyprint 0 "Ejecutando protocolo en [$stage_name]: $script_name"
            source "$install_script" "$BASE_PATH"
        done
    fi
done

# C. (Opcional) Ejecutar scripts sueltos en la raíz de install/$OS/$VERSION_ID/ si existieran
if [ -d "$VERSION_INSTALL_DIR" ]; then
    for install_script in "$VERSION_INSTALL_DIR"/*.sh; do
        [ -f "$install_script" ] || continue
        script_name=$(basename "$install_script")
        prettyprint 0 "Ejecutando protocolo adicional: install/$OS/$VERSION_ID/$script_name"
        source "$install_script" "$BASE_PATH"
    done
fi

prettyprint 0 "Etapas de bootstrapping del entorno completadas."
prettyprint 0 "Iniciando compilación de módulos y binarios con build.sh..."

chmod +x "$BASE_PATH/build.sh"
"$BASE_PATH/build.sh"

prettyprint 0 "¡Configuración por etapas e instalación modular completadas con éxito!"