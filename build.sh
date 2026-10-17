#!/usr/bin/env bash
set -euo pipefail

BASE_PATH="$PWD"

# Cargar funciones de logs compartidas
if [ -f "$BASE_PATH/os/common.sh" ]; then
    source "$BASE_PATH/os/common.sh"
else
    function prettyprint() {
        local level=$1
        shift
        local packed=("$@")

        case $level in
            0)
                printf "\e[0;36m[INFO]:\e[0m %s\n" "${packed[*]}"
            ;;
            1)
                printf "\e[0;33m[WARN]:\e[0m %s\n" "${packed[*]}"
            ;;
            2|*)
                printf "\e[0;31m[FAIL]:\e[0m %s\n" "${packed[*]}"
            ;;
        esac
    }
fi

if ! command -v luajit &>/dev/null; then
    prettyprint 1 "LuaJIT no detectado. Ejecutando bootstrapping de entorno..."
    ./configurarentorno.sh
fi

OUT_DIR="$BASE_PATH/import/Linux"
mkdir -p "$OUT_DIR"

# ==========================================
# FUNCIÓN UNIFICADA DE COMPILACIÓN (clibs y cpplibs)
# ==========================================
function compile_lib() {
    local comp="$1"
    local src_path="$2"
    local out_file="$3"
    local mod_name="$4"
    local base_dir="$5"
    local is_modular="$6"

    prettyprint 0 "Compilando módulo [$comp] $mod_name..."

    local extra_flags=""
    local include_dirs="-I/usr/include/luajit-2.1 -I$base_dir"

    if [ "$comp" = "gcc" ]; then
        extra_flags="-lluajit-5.1"
        if [ "$mod_name" = "cmariadb" ]; then
            local static_lib=$(find /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu -name "libmysqlclient.a" -o -name "libmariadbclient.a" -o -name "libmariadb.a" 2>/dev/null | head -n 1)
            if [ -n "$static_lib" ]; then
                extra_flags="$extra_flags -Wl,--start-group $static_lib -lstdc++ -lssl -lcrypto -lz -lpthread -ldl -lm -Wl,--end-group"
            else
                extra_flags="$extra_flags ${SQL_LDFLAGS:-}"
            fi
            [ -n "${SQL_INC:-}" ] && include_dirs="$include_dirs $SQL_INC"
        elif [ "$mod_name" = "cjson" ]; then
            extra_flags="$extra_flags -ljson-c"
        fi
        extra_flags="$extra_flags $(pkg-config --cflags --libs lua5.1 2>/dev/null || echo "")"
    else
        extra_flags="-lstdc++ -lluajit-5.1"
        if [ "$mod_name" = "raylib" ]; then
            local static_raylib=$(find /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu -name "libraylib.a" 2>/dev/null | head -n 1)
            if [ -n "$static_raylib" ]; then
                extra_flags="$extra_flags $static_raylib -lGL -lm -lpthread -ldl -lrt -lX11"
            else
                extra_flags="$extra_flags -lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
            fi
        fi
        extra_flags="$extra_flags $(pkg-config --cflags --libs luajit 2>/dev/null || echo "")"
    fi

    if [ "$is_modular" = "true" ]; then
        include_dirs="$include_dirs -I$(dirname "$src_path")"
    fi

    "$comp" -O3 -fPIC -shared $src_path $include_dirs -o "$out_file" $extra_flags

    if [ $? -eq 0 ]; then
        prettyprint 0 "OK: $mod_name.so generado en $out_file"
    else
        prettyprint 2 "Fallo compilando el módulo $mod_name"
    fi
}

# ==========================================
# COMPILACIÓN DE CLIBS (C)
# ==========================================
CLIBS_DIR="$BASE_PATH/clibs"
if [ -d "$CLIBS_DIR" ]; then
    prettyprint 0 "Compilando clibs/ (C)..."
    for mod_dir in "$CLIBS_DIR"/*/; do
        [ -d "$mod_dir" ] || continue
        name=$(basename "$mod_dir")
        compile_lib "gcc" "$mod_dir*.c" "$OUT_DIR/$name.so" "$name" "$CLIBS_DIR" "true"
    done

    for src in "$CLIBS_DIR"/*.c; do
        [ -e "$src" ] || continue
        name=$(basename "$src" .c)
        compile_lib "gcc" "$src" "$OUT_DIR/$name.so" "$name" "$CLIBS_DIR" "false"
    done
fi

# ==========================================
# COMPILACIÓN DE CPPLIBS (C++)
# ==========================================
CPPLIBS_DIR="$BASE_PATH/cpplibs"
if [ -d "$CPPLIBS_DIR" ]; then
    prettyprint 0 "Compilando cpplibs/ (C++)..."
    for mod_dir in "$CPPLIBS_DIR"/*/; do
        [ -d "$mod_dir" ] || continue
        name=$(basename "$mod_dir")
        compile_lib "g++" "$mod_dir*.cpp" "$OUT_DIR/$name.so" "$name" "$CPPLIBS_DIR" "true"
    done

    for src in "$CPPLIBS_DIR"/*.cpp; do
        [ -e "$src" ] || continue
        name=$(basename "$src" .cpp)
        compile_lib "g++" "$src" "$OUT_DIR/$name.so" "$name" "$CPPLIBS_DIR" "false"
    done
fi

# ==========================================
# COMPILACIÓN DEL BINARIO BACKEND
# ==========================================
prettyprint 0 "Compilando backend ejecutable..."
g++ -std=c++17 -O3 backend.cpp -I. -lpthread -o backend
prettyprint 0 "¡Compilación completada exitosamente!"