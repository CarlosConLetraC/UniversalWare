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

if ! command -v luajit >/dev/null 2>&1; then
    prettyprint 1 "LuaJIT no encontrado, ejecutando script de configuración..."
    ./configurarentorno.sh
fi

# Compilar el backend en C si no existe el binario o si ha cambiado
[ -x ./backend ] || ./build.sh

# ==========================================
# 1. INICIALIZACIÓN DE LA BASE DE DATOS
# ==========================================
prettyprint 0 "Inicializando base de datos y datos semilla..."
./backend web/lua/ program.main.lua

# ==========================================
# 2. CONFIGURACIÓN Y DESPLIEGUE DE NGINX
# ==========================================
if ! command -v nginx >/dev/null 2>&1; then
    prettyprint 1 "Nginx no encontrado. Instalando Nginx..."
    sudo pacman -S --noconfirm nginx
fi

PROJECT_ROOT="$PWD"
# Ajusta esta ruta si tu index.html está directamente en web/ o en web/public/
HTML_SOURCE="$PROJECT_ROOT/web/index.html" 
NGINX_CONF_SOURCE="$PROJECT_ROOT/web/nginx.conf"
NGINX_WEB_DIR="/var/www/html"

prettyprint 0 "Desplegando configuración de Nginx..."
sudo cp "$NGINX_CONF_SOURCE" /etc/nginx/nginx.conf

prettyprint 0 "Desplegando archivos estáticos hacia el servidor web..."
sudo mkdir -p "$NGINX_WEB_DIR"
if [ -f "$HTML_SOURCE" ]; then
    sudo cp "$HTML_SOURCE" "$NGINX_WEB_DIR/index.html"
else
    prettyprint 1 "No se encontró index.html en $HTML_SOURCE, omitiendo copia automática."
fi

# Permisos seguros estándar para Nginx
sudo chmod 755 /var
sudo chmod 755 /var/www
sudo chmod 755 /var/www/html
[ -f "$NGINX_WEB_DIR/index.html" ] && sudo chmod 644 "$NGINX_WEB_DIR/index.html"

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
# Pasamos la ruta 'web/lua/' como base para que el sistema de 'require' encuentre la carpeta 'fetch/'
exec ./backend web/lua/ program.fetch.lua