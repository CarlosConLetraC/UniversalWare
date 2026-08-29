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

dependencies=(
	luajit wget curl make cmake gfortran gcc g++ build-essential pkg-config
	libssl-dev zlib1g-dev ca-certificates git
	libproj-dev libgeos-dev libgdal-dev
	libblas-dev liblapack-dev
	libwebp-dev protobuf-compiler libprotobuf-dev
	libluajit-5.1-dev libssh2-1-dev
	librsvg2-dev libcurl4-openssl-dev libxml2-dev
	libgit2-dev libjpeg-dev libtiff5-dev libpng-dev
	libfribidi-dev libharfbuzz-dev libcairo2-dev libfontconfig1-dev
	libreadline-dev libncurses-dev unzip zip
	default-libmysqlclient-dev
)

if [ "$OS" = "ubuntu" ]; then
	dependencies+=(libfreetype6-dev)
elif [ "$OS" = "debian" ]; then
	dependencies+=(libfreetype-dev)
else
	prettyprint 2 "Distribucion no soportada: $OS"
	exit 1
fi

prettyprint 0 "Instalando resto de dependencias sin recomendaciones de MySQL. . ."
sudo apt-get update
sudo apt-get install -y --no-install-recommends "${dependencies[@]}"

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

prettyprint 0 "Compilando clibs/ . . ."

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

prettyprint 0 "Banderas de enlace SQL detectadas: $SQL_LDFLAGS"

# Compilar módulos multianchivo en subdirectorios (ej: clibs/cmariadb/, clibs/cjob/)
for mod_dir in "$CLIBS_DIR"/*/; do
    [ -d "$mod_dir" ] || continue
    name=$(basename "$mod_dir")
    out="$OUT_DIR/$name.so"
    prettyprint 0 "Compilando módulo modular: $name/ . . ."

    EXTRA_FLAGS=""
    if [ "$name" = "cmariadb" ]; then
        # Buscar librería estática en el sistema
        STATIC_LIB=$(find /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu -name "libmysqlclient.a" -o -name "libmariadbclient.a" -o -name "libmariadb.a" 2>/dev/null | head -n 1)

        if [ -n "$STATIC_LIB" ]; then
            prettyprint 0 "Incrustando biblioteca estática: $STATIC_LIB"
            # -Wl,--start-group / --end-group resuelve referencias cruzadas no ordenadas
            EXTRA_FLAGS="-Wl,--start-group $STATIC_LIB -lstdc++ -lssl -lcrypto -lz -lpthread -ldl -lm -Wl,--end-group"
        else
            prettyprint 1 "No se encontró .a estático, usando enlace dinámico detectado: $SQL_LDFLAGS"
            EXTRA_FLAGS="$SQL_LDFLAGS"
        fi
    fi

    # Compilación genérica de módulos C en subcarpetas
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
        prettyprint 0 "OK: $name.so generado en $out"
    else
        prettyprint 2 "Fallo compilando el módulo $name"
    fi
done

# Compilar archivos .c simples sueltos en la raíz de clibs/
for src in "$CLIBS_DIR"/*.c; do
	[ -e "$src" ] || continue
	name=$(basename "$src" .c)
	out="$OUT_DIR/$name.so"
	prettyprint 0 "Compilando módulo simple: $name. . ."

	gcc -O3 -fPIC -shared "$src" \
		-I/usr/include/luajit-2.1 \
		-I"$CLIBS_DIR" \
		-o "$out" \
		-lluajit-5.1 \
		$(pkg-config --cflags --libs lua5.1 2>/dev/null || echo "")

	if [ $? -eq 0 ]; then
		prettyprint 0 "OK: $name.so generado"
	else
		prettyprint 2 "Fallo compilando $name"
	fi
done

prettyprint 0 "Compilando cpplibs/*.cpp . . ."
CPPLIBS_DIR="$BASE_PATH/cpplibs"

if [ -d "$CPPLIBS_DIR" ]; then
	for src in "$CPPLIBS_DIR"/*.cpp; do
		[ -e "$src" ] || continue

		name=$(basename "$src" .cpp)
		out="$OUT_DIR/$name.so"

		prettyprint 0 "Compilando $name (C++). . ."

		g++ -O3 -fPIC \
			-I/usr/include/luajit-2.1 \
			-shared "$src" \
			-o "$out" \
			-lluajit-5.1 \
			-lstdc++ \
			$(pkg-config --cflags --libs luajit 2>/dev/null || echo "")

		if [ $? -eq 0 ]; then
			prettyprint 0 "OK: $name.so generado"
		else
			prettyprint 2 "Fallo compilando $name"
		fi
	done
else
	prettyprint 2 "No existe o no se pudo crear directorio cpplibs/"
	exit 1
fi

prettyprint 0 "Instalacion completada correctamente."
