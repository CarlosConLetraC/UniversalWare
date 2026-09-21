#!/usr/bin/env bash
set -euo pipefail

function prettyprint() {
    local level=$1
    shift
    local packed=("$@")

    case $level in
        0) printf "\e[0;36m[INFO]:\e[0m %s\n" "${packed[*]}" ;;
        1) printf "\e[0;33m[WARN]:\e[0m %s\n" "${packed[*]}" ;;
        2|*) printf "\e[0;31m[FAIL]:\e[0m %s\n" "${packed[*]}" ;;
    esac
}

if [ ! -f "/usr/bin/luajit" ] || ! luajit -v | grep -q "2.1.178"; then
	prettyprint 0 "Compilando la versión más reciente de LuaJIT desde la fuente oficial. . ."
	cd /tmp
	git clone https://github.com/LuaJIT/LuaJIT.git
	cd LuaJIT
	make PREFIX=/usr
	sudo make install PREFIX=/usr
	cd /tmp
	rm -rf LuaJIT
fi

# Compilar el backend en C si no existe el binario o si ha cambiado
[ -x ./backend ] || ./build.sh

# ==========================================
# 1. INICIALIZACIÓN DE LA BASE DE DATOS
# ==========================================
prettyprint 0 "Inicializando base de datos y datos semilla..."
./runclient program.main.lua

# ==========================================
# 2. CONFIGURACIÓN Y DESPLIEGUE DE NGINX
# ==========================================
if ! command -v nginx >/dev/null 2>&1; then
    prettyprint 1 "Nginx no encontrado. Instalando Nginx..."
    sudo pacman -S --noconfirm nginx
fi

PROJECT_ROOT="$PWD"
PUBLIC_DIR="$PROJECT_ROOT/web/public"
NGINX_CONF_SOURCE="$PROJECT_ROOT/web/nginx.conf"
NGINX_WEB_DIR="/var/www/html"

prettyprint 0 "Desplegando configuración de Nginx..."
sudo cp "$NGINX_CONF_SOURCE" /etc/nginx/nginx.conf

prettyprint 0 "Desplegando todos los recursos estáticos (HTML, JS, CSS) desde web/public..."
sudo mkdir -p "$NGINX_WEB_DIR"

if [ -d "$PUBLIC_DIR" ]; then
    sudo cp -r "$PUBLIC_DIR"/* "$NGINX_WEB_DIR/"
else
    prettyprint 1 "No se encontró el directorio $PUBLIC_DIR"
fi

# Permisos de lectura y ejecución para el usuario web
sudo chmod 755 /var
sudo chmod 755 /var/www
sudo chmod 755 /var/www/html
sudo chmod -R 644 "$NGINX_WEB_DIR"/* || true

# Validar y reiniciar Nginx
if sudo nginx -t >/dev/null 2>&1; then
    sudo fuser -k 8081/tcp || true
    sudo pkill -9 nginx || true
    sudo systemctl restart nginx
    prettyprint 0 "Nginx configurado, desplegado y reiniciado correctamente."
else
    prettyprint 2 "Error en la sintaxis de la configuración de Nginx."
    exit 1
fi

# ==========================================
#  3. LANZAMIENTO DEL BACKEND MODULAR
# ==========================================
prettyprint 0 "Iniciando servidor API modular en C/LuaJIT..."
exec ./backend web/lua/ program.fetch.lua