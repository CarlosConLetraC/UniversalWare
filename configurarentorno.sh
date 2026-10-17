#!/usr/bin/env bash
set -euo pipefail

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

prettyprint 0 "Iniciando instalacion. . ."

export MAKEFLAGS="-j$(nproc || echo 2)"
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig"

BASE_PATH="$PWD"
LUAROCKS_VERSION="3.13.0"
CONFIG_DEPENDENCIES="$BASE_PATH/dependencies.conf"

source /etc/os-release
OS=$ID
CODENAME="${VERSION_CODENAME:-}"

if [ -z "$CODENAME" ]; then
	prettyprint 2 " VERSION_CODENAME vacio."
	exit 1
fi

prettyprint 0 "OS: $OS ($CODENAME)"

mongod --version > /dev/null 2>&1 && MONGOD_INSTALADO=0 || MONGOD_INSTALADO=1
luarocks --version > /dev/null 2>&1 && LUAROCKS_INSTALADO=0 || LUAROCKS_INSTALADO=1

# Leer dependencias desde el archivo de configuración externo
if [ ! -f "$CONFIG_DEPENDENCIES" ]; then
    prettyprint 2 "No se encuentra el archivo de configuración de dependencias: $CONFIG_DEPENDENCIES"
    exit 1
fi

dependencies=()
while IFS= read -r line; do
    # Limpiar espacios y omitir comentarios o líneas vacías
    line=$(echo "$line" | sed 's/#.*//' | xargs)
    [ -n "$line" ] && dependencies+=("$line")
done < "$CONFIG_DEPENDENCIES"

# Dependencias específicas por distribución
if [ "$OS" = "ubuntu" ]; then
	dependencies+=(libfreetype6-dev libcjson-dev lua-cjson)
elif [ "$OS" = "debian" ]; then
	dependencies+=(libfreetype-dev libcjson-dev lua-cjson)
else
	prettyprint 2 "Distribucion no soportada: $OS"
	exit 1
fi

prettyprint 0 "Instalando dependencias desde configuración. . ."
sudo apt-get update
sudo apt-get install -y --no-install-recommends "${dependencies[@]}"

# Forzar la compilación e instalación de la última versión de LuaJIT desde el código fuente
if [ ! -f "/usr/bin/luajit" ] || ! luajit -v | grep -q "2.1.178"; then
	prettyprint 0 "Compilando la versión más reciente de LuaJIT desde la fuente oficial. . ."
	sudo apt-get remove -y libluajit-5.1-dev luajit || true

	cd /tmp
	git clone https://github.com/LuaJIT/LuaJIT.git
	cd LuaJIT

	make PREFIX=/usr
	sudo make install PREFIX=/usr

	cd /tmp
	rm -rf LuaJIT
fi

if [ "$LUAROCKS_INSTALADO" -ne 0 ]; then
	prettyprint 0 "Instalando LuaRocks $LUAROCKS_VERSION. . ."

	cd /tmp || exit 1
	rm -rf "luarocks-$LUAROCKS_VERSION"

	wget -q "https://luarocks.org/releases/luarocks-$LUAROCKS_VERSION.tar.gz"
	tar zxf "luarocks-$LUAROCKS_VERSION.tar.gz"
	cd "luarocks-$LUAROCKS_VERSION"

	./configure \
		--with-lua-include=/usr/include/luajit-2.1 \
		--with-lua-bin=/usr/bin \
		--lua-suffix=jit \
		--lua-version=5.1

	make
	sudo make install
	cd ..

	rm -rf "luarocks-$LUAROCKS_VERSION"
fi

prettyprint 0 "Instalando paquetes Lua. . ."
sudo luarocks install luasocket || true
sudo luarocks install luasec || true

if [ ! -f "$BASE_PATH/import/Linux/ssh.so" ]; then
	prettyprint 0 "Compilando lua-ssh. . ."

	cd /tmp
	rm -rf lua-ssh
	git clone https://github.com/esno/lua-ssh.git
	cd lua-ssh/src

	gcc -O2 -fPIC -I/usr/include/luajit-2.1 -c ssh.c -o ssh.o
	gcc -shared -o ssh.so ssh.o -lluajit-5.1 $(pkg-config --libs libssh2 || echo "-lssh2")

	mkdir -p "$BASE_PATH/import/Linux/"
	cp ssh.so "$BASE_PATH/import/Linux/"

	cd /tmp
	rm -rf lua-ssh
fi

prettyprint 0 "Compilando clibs/ (C) . . ."

CLIBS_DIR="$BASE_PATH/clijs" # Corregido a CLIBS_DIR
CLIBS_DIR="$BASE_PATH/clibs"
OUT_DIR="$BASE_PATH/import/Linux"

mkdir -p "$OUT_DIR"

if [ ! -d "$CLIBS_DIR" ]; then
    prettyprint 2 "No existe o no se pudo crear directorio clibs/"
    exit 1
fi

# 1. Detección de cabeceras de MySQL/MariaDB en el sistema
SQL_INC=""
for header in $(find /usr/include -name "mysql.h" -o -name "mariadb.h" 2>/dev/null); do
    dir=$(dirname "$header")
    parent_dir=$(dirname "$dir")
    
    [[ "$SQL_INC" != *"-I$dir"* ]] && SQL_INC="$SQL_INC -I$dir"
    [[ "$SQL_INC" != *"-I$parent_dir"* ]] && SQL_INC="$SQL_INC -I$parent_dir"
done

if [ -z "$SQL_INC" ]; then
    SQL_INC="-I/usr/include/mysql -I/usr/include/mariadb -I/usr/include"
fi

# 2. Detección de la bandera de enlace dinámica para MariaDB / MySQL
SQL_LDFLAGS=""
if pkg-config --libs mariadb 2>/dev/null | grep -q -- "-l"; then
    SQL_LDFLAGS=$(pkg-config --libs mariadb)
elif pkg-config --libs mysqlclient 2>/dev/null | grep -q -- "-l"; then
    SQL_LDFLAGS=$(pkg-config --libs mysqlclient)
elif [ -f /usr/lib/x86_64-linux-gnu/libmariadb.so ] || [ -f /usr/lib/libmariadb.so ]; then
    SQL_LDFLAGS="-lmariadb"
else
    SQL_LDFLAGS="-lmysqlclient"
fi

# Compilar módulos multianchivo en subdirectorios de clibs/
for mod_dir in "$CLIBS_DIR"/*/; do
    [ -d "$mod_dir" ] || continue
    name=$(basename "$mod_dir")
    out="$OUT_DIR/$name.so"
    prettyprint 0 "Compilando módulo C modular: $name/ . . ."

    EXTRA_FLAGS=""
    if [ "$name" = "cmariadb" ]; then
        STATIC_LIB=$(find /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu -name "libmysqlclient.a" -o -name "libmariadbclient.a" -o -name "libmariadb.a" 2>/dev/null | head -n 1)

        if [ -n "$STATIC_LIB" ]; then
            EXTRA_FLAGS="-Wl,--start-group $STATIC_LIB -lstdc++ -lssl -lcrypto -lz -lpthread -ldl -lm -Wl,--end-group"
        else
            EXTRA_FLAGS="$SQL_LDFLAGS"
        fi
    fi

    gcc -O3 -fPIC -shared \
        "$mod_dir"*.c \
        -I/usr/include/luajit-2.1 \
        -I"$CLIBS_DIR" \
        -I"$mod_dir" \
        ${SQL_INC:-} \
        -o "$out" \
        -lluajit-5.1 \
        $EXTRA_FLAGS \
        $(pkg-config --cflags --libs lua5.1 2>/dev/null || echo "")

    if [ $? -eq 0 ]; then
        prettyprint 0 "OK: $name.so generado"
    else
        prettyprint 2 "Fallo compilando el módulo C $name"
    fi
done

# Compilar archivos .c simples sueltos en la raíz de clibs/
for src in "$CLIBS_DIR"/*.c; do
	[ -e "$src" ] || continue
	name=$(basename "$src" .c)
	out="$OUT_DIR/$name.so"
	prettyprint 0 "Compilando módulo C simple: $name. . ."

	EXTRA_FLAGS=""
	if [ "$name" = "cjson" ]; then
		EXTRA_FLAGS="-ljson-c"
	fi

	gcc -O3 -fPIC -shared "$src" \
		-I/usr/include/luajit-2.1 \
		-I"$CLIBS_DIR" \
		-o "$out" \
		-lluajit-5.1 \
		$EXTRA_FLAGS \
		$(pkg-config --cflags --libs lua5.1 2>/dev/null || echo "")

	if [ $? -eq 0 ]; then
		prettyprint 0 "OK: $name.so generado"
	else
		prettyprint 2 "Fallo compilando $name"
	fi
done

# ==========================================
# COMPILACIÓN DE CPPLIBS (C++) - Espejo exacto de clibs
# ==========================================
prettyprint 0 "Compilando cpplibs/ (C++) . . ."
CPPLIBS_DIR="$BASE_PATH/cpplibs"

if [ -d "$CPPLIBS_DIR" ]; then
    # 1. Compilar subdirectorios modulares con múltiples archivos .cpp (Ej: cpplibs/raylib/)
    for mod_dir in "$CPPLIBS_DIR"/*/; do
        [ -d "$mod_dir" ] || continue
        name=$(basename "$mod_dir")
        out="$OUT_DIR/$name.so"
        prettyprint 0 "Compilando módulo C++ modular: $name/ . . ."

        # Flags adicionales específicos si es necesario (ej: Raylib requiere -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 si aplica)
        EXTRA_CPP_FLAGS="-lstdc++"
        if [ "$name" = "raylib" ]; then
            # Ajusta o añade las dependencias nativas de raylib si compilas desde fuentes o usas la librería del sistema
            if pkg-config --exists raylib 2>/dev/null; then
                EXTRA_CPP_FLAGS="$EXTRA_CPP_FLAGS $(pkg-config --libs raylib)"
            else
                EXTRA_CPP_FLAGS="$EXTRA_CPP_FLAGS -lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
            fi
        fi

        g++ -O3 -fPIC -shared \
            "$mod_dir"*.cpp \
            -I/usr/include/luajit-2.1 \
            -I"$CPPLIBS_DIR" \
            -I"$mod_dir" \
            -o "$out" \
            -lluajit-5.1 \
            $EXTRA_CPP_FLAGS \
            $(pkg-config --cflags --libs luajit 2>/dev/null || echo "")

        if [ $? -eq 0 ]; then
            prettyprint 0 "OK: $name.so (C++) generado"
        else
            prettyprint 2 "Fallo compilando el módulo C++ modular $name"
        fi
    done

    # 2. Compilar archivos .cpp individuales sueltos en la raíz de cpplibs/
    for src in "$CPPLIBS_DIR"/*.cpp; do
        [ -e "$src" ] || continue
        name=$(basename "$src" .cpp)
        out="$OUT_DIR/$name.so"
        prettyprint 0 "Compilando módulo C++ simple: $name. . ."

        g++ -O3 -fPIC -shared "$src" \
            -I/usr/include/luajit-2.1 \
            -I"$CPPLIBS_DIR" \
            -o "$out" \
            -lluajit-5.1 \
            -lstdc++ \
            $(pkg-config --cflags --libs luajit 2>/dev/null || echo "")

        if [ $? -eq 0 ]; then
            prettyprint 0 "OK: $name.so (C++) generado"
        else
            prettyprint 2 "Fallo compilando el módulo C++ simple $name"
        fi
    done
else
	prettyprint 2 "No existe o no se pudo crear directorio cpplibs/"
	exit 1
fi

prettyprint 0 "Instalacion completada correctamente."