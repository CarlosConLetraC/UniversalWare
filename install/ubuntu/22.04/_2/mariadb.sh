#!/usr/bin/env bash
BASE_PATH="${1:-$PWD}"
source "$BASE_PATH/os/common.sh"

prettyprint 0 "Configurando rutas de cabeceras y flags para MariaDB/MySQL..."

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

export SQL_INC
export SQL_LDFLAGS
prettyprint 0 "Configuración de MariaDB completada."