#!/usr/bin/env bash
set -euo pipefail
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

LUAROCKS_VERSION="3.13.0"
luarocks --version > /dev/null 2>&1 && LUAROCKS_INSTALADO=0 || LUAROCKS_INSTALADO=1

LUAJIT_RAW_VER=$(luajit -v 2>&1 | awk '{print $2}')

if [ ! -f "/usr/bin/luajit" ] || [[ "$LUAJIT_RAW_VER" < "2.1.178" ]]; then
    prettyprint 1 "Se necesita de LuaJIT >= 2.1.178. Versión actual: '$LUAJIT_RAW_VER'."
	prettyprint 0 "Compilando la versión más reciente de LuaJIT desde la fuente oficial..."
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
	prettyprint 0 "Instalando LuaRocks $LUAROCKS_VERSION..."

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

prettyprint 0 "Instalando paquetes esenciales de Lua (luasocket, luasec)..."
sudo luarocks install luasocket || true
sudo luarocks install luasec || true