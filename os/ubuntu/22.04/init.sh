#!/usr/bin/env bash
set -euo pipefail

BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Iniciando instalación de dependencias del sistema para Ubuntu 22.04..."

# Apuntar directamente a la ruta jerárquica de dependencias de Ubuntu 22.04
CONFIG_DIR="$BASE_PATH/dependencies/ubuntu/22.04"
if [ ! -d "$CONFIG_DIR" ]; then
    prettyprint 2 "No se encuentra el directorio de dependencias: $CONFIG_DIR"
    exit 1
fi

all_dependencies=()
for conf_file in "$CONFIG_DIR"/*.conf; do
    [ -f "$conf_file" ] || continue
    while IFS= read -r line; do
        # Limpiar espacios y omitir comentarios o líneas vacías
        line=$(echo "$line" | sed 's/#.*//' | xargs)
        [ -n "$line" ] && all_dependencies+=("$line")
    done < "$conf_file"
done

if [ ${#all_dependencies[@]} -gt 0 ]; then
    prettyprint 0 "Actualizando repositorios e instalando paquetes APT definidos en dependencies/ubuntu/22.04/..."
    sudo apt-get update
    # Se instalan herramientas esenciales, librerías gráficas y demás submódulos declarados
    sudo apt-get install -y --no-install-recommends "${all_dependencies[@]}"
else
    prettyprint 1 "No se encontraron dependencias en $CONFIG_DIR"
fi

prettyprint 0 "Dependencias del sistema instaladas correctamente para Ubuntu 22.04."