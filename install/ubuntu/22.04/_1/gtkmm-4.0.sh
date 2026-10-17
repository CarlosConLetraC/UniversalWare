#!/usr/bin/env bash
set -euo pipefail
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Iniciando compilación de la cadena GTK / gtkmm desde el código fuente (Etapa _1)..."

export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:${PKG_CONFIG_PATH:-}"

# Función auxiliar inteligente para compilar vía Meson o Autotools
build_gnome_tarball() {
    local package_name="$1"
    local version="$2"
    local pkg_config_name="$3"
    local remote_name="${4:-$package_name}" # Nombre real en download.gnome.org si difiere
    
    if pkg-config --exists "$pkg_config_name"; then
        prettyprint 0 "$pkg_config_name ya se encuentra disponible en el sistema."
        return 0
    fi

    prettyprint 0 "Descargando y compilando $package_name $version..."
    cd /tmp
    rm -rf "${remote_name}-${version}" "${package_name}-${version}"
    
    local major_version=$(echo "$version" | cut -d. -f1,2)
    local url="https://download.gnome.org/sources/${remote_name}/${major_version}/${remote_name}-${version}.tar.xz"
    
    if ! wget -q "$url"; then
        prettyprint 2 "Fallo al descargar el tarball desde $url"
        exit 1
    fi
    
    tar -xf "${remote_name}-${version}.tar.xz"
    cd "${remote_name}-${version}"
    
    if [ -f "meson.build" ]; then
        mkdir -p build && cd build
        meson setup .. --prefix=/usr --libdir=lib/x86_64-linux-gnu -Dbuildtype=release -Dbuild-documentation=false
        ninja
        sudo ninja install
    else
        ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
        make -j$(nproc || echo 2)
        sudo make install
    fi
    
    cd /tmp
    rm -rf "${remote_name}-${version}" "${remote_name}-${version}.tar.xz"
    prettyprint 0 "OK: $package_name $version instalado correctamente."
}

# 1. libsigc++ (versión estable 3.4.0)
build_gnome_tarball "libsigcplusplus" "3.4.0" "sigc++-3.0" "libsigc++"

# 2. glibmm (versión estable 2.68.2)
build_gnome_tarball "glibmm" "2.68.2" "glibmm-2.68"

# 3. cairomm (versión 1.15.4 - Usa Autotools)
build_gnome_tarball "cairomm" "1.15.4" "cairomm-1.16"

# 4. pangomm (versión estable 2.48.2)
build_gnome_tarball "pangomm" "2.48.2" "pangomm-2.48"

# 5. gtkmm (versión estable 4.6.1)
build_gnome_tarball "gtkmm" "4.6.1" "gtkmm-4.0"