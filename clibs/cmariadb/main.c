#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "datatypes.h"
#include "clientmodes.h"

#define MARIADB_LUA_METATABLE "MariaDB.Connection"

// Estructura contenedora para el Userdata de Lua
typedef struct {
    MYSQL *conn;
} LuaMariaDB;

// Metametodo __tostring: Preserva la representacion en texto exacto (ej: "30.00")
static int sqlvalue_tostring(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    lua_pushstring(L, v->str_repr);
    return 1;
}

// Metodo to_number(): Castea explicitamente a number de Lua
static int sqlvalue_tonumber(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    lua_pushnumber(L, v->num_val);
    return 1;
}

// Metodo type(): Devuelve el nombre del tipo de columna de la base de datos
/*static int sqlvalue_type(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    switch (v->sql_type) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            lua_pushstring(L, "DECIMAL");
            break;
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
            lua_pushstring(L, "INTEGER");
            break;
        case MYSQL_TYPE_LONGLONG:
            lua_pushstring(L, "BIGINT");
            break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
            lua_pushstring(L, "FLOAT");
            break;
        default:
            lua_pushstring(L, "UNKNOWN");
            break;
    }
    return 1;
}*/
static int sqlvalue_type(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    switch (v->sql_type) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            lua_pushstring(L, "DECIMAL");
            break;
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
            lua_pushstring(L, "INTEGER");
            break;
        case MYSQL_TYPE_LONGLONG:
            lua_pushstring(L, "BIGINT");
            break;
        case MYSQL_TYPE_FLOAT:
            lua_pushstring(L, "FLOAT");
            break;
        case MYSQL_TYPE_DOUBLE:
            lua_pushstring(L, "DOUBLE");
            break;
        case MYSQL_TYPE_BIT:
            lua_pushstring(L, "BIT");
            break;
        default:
            lua_pushstring(L, "UNKNOWN");
            break;
    }
    return 1;
}

// Registra la metatabla de SqlValue en el estado de Lua
void register_sqlvalue_meta(lua_State *L) {
    luaL_newmetatable(L, SQLVALUE_META);

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_newtable(L);

    lua_pushcfunction(L, sqlvalue_tonumber);
    lua_setfield(L, -2, "tonumber");

    lua_pushcfunction(L, sqlvalue_type);
    lua_setfield(L, -2, "type");

    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

// Mapea la columna recibida de MariaDB al tipo correspondiente en Lua
void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length) {
    if (!val) {
        lua_pushnil(L);
        return;
    }

    switch (field->type) {
        // Tipos de precision fija / decimales / BigInt -> SqlValue
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_LONGLONG: {
            SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
            v->sql_type = field->type;
            v->num_val = atof(val);

            size_t copy_len = length < sizeof(v->str_repr) - 1 ? length : sizeof(v->str_repr) - 1;
            memcpy(v->str_repr, val, copy_len);
            v->str_repr[copy_len] = '\0';

            luaL_getmetatable(L, SQLVALUE_META);
            lua_setmetatable(L, -2);
            break;
        }

        // Enteros estandar -> Lua Integer
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
            lua_pushinteger(L, atoi(val));
            break;

        // Punto flotante estandar -> Lua Number
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
            lua_pushnumber(L, atof(val));
            break;

        // Cadenas y texto general -> Lua String
        default:
            lua_pushlstring(L, val, length);
            break;
    }
}

// [ANTES] Firma: mariadb.connect("host", "user", "pass", "db" [opt], "port" [opt: 3306], "client_mode" [opt], "socket" [opt])
// Firma 1: mariadb.connect("host", "user", "pass", "db" [opt], "port" [opt], "client_mode" [opt], "socket" [opt])
// Firma 2: mariadb.connect({ host = "...", user = "...", password = "...", db = "...", port = 3306, client_mode = ..., socket = "..." })
static int l_connect(lua_State *L) {
    const char *host = NULL,
               *user = NULL,
               *pass = NULL,
               *db   = NULL;
    int port         = 3306;
    unsigned long client_flags = MARIADB_CLIENT_DEFAULT_MODE;
    const char *socket_path = NULL;

    // --- MODO 1: Parámetro de tipo Tabla ---
    if (lua_istable(L, 1)) {
        // 1. host
        lua_getfield(L, 1, "host");
        if (lua_isstring(L, -1)) host = lua_tostring(L, -1);
        lua_pop(L, 1);

        // 2. user
        lua_getfield(L, 1, "user");
        if (lua_isstring(L, -1)) user = lua_tostring(L, -1);
        lua_pop(L, 1);

        // 3. password (soporta 'password' o 'pass')
        lua_getfield(L, 1, "password");
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            lua_getfield(L, 1, "pass");
        }
        if (lua_isstring(L, -1)) pass = lua_tostring(L, -1);
        lua_pop(L, 1);

        // Validar campos obligatorios en tabla
        if (!host || !user || !pass) {
            return luaL_error(L, "La tabla de conexión requiere las claves 'host', 'user' y 'password'");
        }

        // 4. db (opcional)
        lua_getfield(L, 1, "db");
        if (lua_isstring(L, -1)) db = lua_tostring(L, -1);
        lua_pop(L, 1);

        // 5. port (opcional, default 3306)
        lua_getfield(L, 1, "port");
        if (lua_isnumber(L, -1)) port = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        // 6. client_mode (opcional)
        lua_getfield(L, 1, "client_mode");
        if (lua_isnumber(L, -1)) {
            int custom_mode = (int)lua_tointeger(L, -1);
            if (custom_mode == MARIADB_CLIENT_DEFAULT_MODE || custom_mode == MARIADB_CLIENT_MULTI_STATEMENTS) {
                client_flags = (unsigned long)custom_mode;
            } else {
                lua_pop(L, 1);
                return luaL_error(L, "client_mode no permitido: %d", custom_mode);
            }
        }
        lua_pop(L, 1);

        // 7. socket (opcional)
        lua_getfield(L, 1, "socket");
        if (lua_isstring(L, -1)) socket_path = lua_tostring(L, -1);
        lua_pop(L, 1);

    // --- MODO 2: Parámetros Posicionales Tradicionales ---
    } else {
        host = luaL_checkstring(L, 1);
        user = luaL_checkstring(L, 2);
        pass = luaL_checkstring(L, 3);
        db   = luaL_optstring(L, 4, NULL);
        port = (int)luaL_optinteger(L, 5, 3306);

        if (lua_gettop(L) >= 6 && !lua_isnil(L, 6)) {
            int custom_mode = (int)luaL_checkinteger(L, 6);
            switch (custom_mode) {
                case MARIADB_CLIENT_DEFAULT_MODE:
                case MARIADB_CLIENT_MULTI_STATEMENTS:
                    break;
                default:
                    return luaL_error(L, "client_mode no permitido: %d", custom_mode);
            }
            client_flags = (unsigned long)custom_mode;
        }

        socket_path = luaL_optstring(L, 7, NULL);
    }

    // Limpieza de cadenas vacías
    if (db && strlen(db) == 0) db = NULL;
    if (socket_path && strlen(socket_path) == 0) socket_path = NULL;

    // --- Conexión con MariaDB ---
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        lua_pushnil(L);
        lua_pushstring(L, "Error al inicializar la estructura MYSQL");
        return 2;
    }

    mysql_options(conn, MYSQL_READ_DEFAULT_GROUP, "client");

    if (!mysql_real_connect(conn, host, user, pass, db, port, socket_path, client_flags)) {
        lua_pushnil(L);
        lua_pushstring(L, mysql_error(conn));
        mysql_close(conn);
        return 2;
    }

    LuaMariaDB *db_obj = (LuaMariaDB *)lua_newuserdata(L, sizeof(LuaMariaDB));
    db_obj->conn = conn;

    luaL_getmetatable(L, MARIADB_LUA_METATABLE);
    lua_setmetatable(L, -2);

    return 1;
}

// 2. db:query("SELECT ...")[cite: 1]
static int l_query(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    const char *sql = luaL_checkstring(L, 2);

    if (!db_obj->conn)
        return luaL_error(L, "La conexion a la base de datos esta cerrada");

    if (mysql_query(db_obj->conn, sql) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, mysql_error(db_obj->conn));
        return 2;
    }

    MYSQL_RES *res = mysql_store_result(db_obj->conn);
    if (!res) {
        // Para consultas que no devuelven filas (INSERT, UPDATE, DELETE)[cite: 1]
        lua_pushboolean(L, 1);
        return 1;
    }

    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD *fields = mysql_fetch_fields(res);

    // Crear tabla principal para los resultados en Lua {{col1=val1}, {col1=val2}}[cite: 1]
    lua_newtable(L);
    int row_index = 1;

    MYSQL_ROW row;
    unsigned long *lengths;

    while ((row = mysql_fetch_row(res))) {
        lua_newtable(L); // Tabla para la fila actual[cite: 1]
        lengths = mysql_fetch_lengths(res);

        for (int i = 0; i < num_fields; i++) {
            lua_pushstring(L, fields[i].name); // Clave (nombre de la columna)[cite: 1]
            
            // Insercion con conversion de tipos SqlValue / primitivos
            push_mariadb_field(L, &fields[i], row[i], lengths ? lengths[i] : 0);
            lua_settable(L, -3); // fila[nombre_columna] = valor[cite: 1]
        }

        lua_rawseti(L, -2, row_index++); // resultados[row_index] = fila[cite: 1]
    }

    mysql_free_result(res);
    return 1;
}

// Método que tokeniza el script por ';' respetando comillas y ejecuta sentencia por sentencia
static int l_multi_query(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    const char *sql_script = luaL_checkstring(L, 2);

    if (!db_obj->conn)
        return luaL_error(L, "La conexion a la base de datos esta cerrada");

    lua_newtable(L); // Tabla para los resultados de retorno en Lua
    int statement_index = 1;

    const char *p = sql_script;
    const char *stmt_start = p;

    char in_quote = 0; // Guarda si estamos dentro de ' o " o `

    while (*p != '\0') {
        char c = *p;

        // 1. Manejo de comillas ('', "", ``)
        if (c == '\'' || c == '"' || c == '`') {
            if (in_quote == 0) {
                in_quote = c; // Entramos en comillas
            } else if (in_quote == c) {
                // Verificamos que no sea un escape (ej: \')
                if (*(p - 1) != '\\')
                    in_quote = 0; // Salimos de comillas
            }
        }
        // 2. Si encontramos ';' Y no estamos dentro de comillas -> Fin de sentencia
        else if (c == ';' && in_quote == 0) {
            size_t len = p - stmt_start;

            if (len > 0) {
                // Copiamos la sentencia a un buffer temporal nulo-terminado
                char *single_stmt = (char *)malloc(len + 1);
                memcpy(single_stmt, stmt_start, len);
                single_stmt[len] = '\0';

                // Verificamos si la sentencia no está vacía o llena de espacios
                char *p_trim = single_stmt;
                while (*p_trim == ' ' || *p_trim == '\t' || *p_trim == '\n' || *p_trim == '\r')
                    p_trim++;

                if (*p_trim != '\0') {
                    // Ejecutamos la consulta individual con la API de MariaDB
                    if (mysql_query(db_obj->conn, single_stmt) != 0) {
                        lua_pushnil(L);
                        lua_pushfstring(L, "Error en sentencia #%d ('%s'): %s", statement_index, single_stmt, mysql_error(db_obj->conn));
                        free(single_stmt);
                        return 2;
                    }

                    MYSQL_RES *res = mysql_store_result(db_obj->conn);
                    if (res) {
                        // Es un SELECT -> Guardar filas
                        int num_fields = mysql_num_fields(res);
                        MYSQL_FIELD *fields = mysql_fetch_fields(res);

                        lua_newtable(L);
                        int row_index = 1;
                        MYSQL_ROW row;
                        unsigned long *lengths;

                        while ((row = mysql_fetch_row(res))) {
                            lua_newtable(L);
                            lengths = mysql_fetch_lengths(res);

                            for (int i = 0; i < num_fields; i++) {
                                lua_pushstring(L, fields[i].name);
                                push_mariadb_field(L, &fields[i], row[i], lengths ? lengths[i] : 0);
                                lua_settable(L, -3);
                            }
                            lua_rawseti(L, -2, row_index++);
                        }
                        mysql_free_result(res);
                        lua_rawseti(L, -2, statement_index++);
                    } else {
                        // Es DDL/DML -> Guardar true
                        if (mysql_field_count(db_obj->conn) == 0) {
                            lua_pushboolean(L, 1);
                            lua_rawseti(L, -2, statement_index++);
                        } else {
                            lua_pushnil(L);
                            lua_pushstring(L, mysql_error(db_obj->conn));
                            free(single_stmt);
                            return 2;
                        }
                    }
                }
                free(single_stmt);
            }
            stmt_start = p + 1; // Avanzar el puntero de inicio para la siguiente sentencia
        }
        p++;
    }

    return 1;
}

// 3. Destructor (__gc): Garantiza cerrar la conexion al recolectar el objeto[cite: 1]
static int l_close(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    if (db_obj->conn) {
        mysql_close(db_obj->conn);
        db_obj->conn = NULL;
    }
    return 0;
}

// Metodos asociados a la metatable[cite: 1]
static const struct luaL_Reg db_methods[] = {
    {"query",       l_query},
    {"multi_query", l_multi_query},
    {"close",       l_close},
    {"__gc",        l_close},
    {NULL, NULL}
};

// Punto de entrada del modulo C[cite: 1]
int luaopen_cmariadb(lua_State *L) {
    // 1. Registrar el metametodo/metatabla de SqlValue
    register_sqlvalue_meta(L);

    // 2. Crear y registrar la metatabla para la conexion de MariaDB (Resuelve attempt to index a userdata)
    if (luaL_newmetatable(L, MARIADB_LUA_METATABLE)) {
        luaL_setfuncs(L, db_methods, 0);

        // Permitir la busqueda de metodos via __index (db:query)
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);

    // 3. Crear la tabla del modulo 'cmariadb'
    luaL_Reg functions[] = {
        {"connect", l_connect},
        {NULL, NULL}
    };
    
    luaL_newlib(L, functions);

    // 4. Exponer la subtabla CLIENT_MODE en Lua[cite: 1]
    lua_newtable(L);
    
    lua_pushinteger(L, MARIADB_CLIENT_DEFAULT_MODE);
    lua_setfield(L, -2, "DEFAULT");

    lua_pushinteger(L, MARIADB_CLIENT_MULTI_STATEMENTS);
    lua_setfield(L, -2, "MULTI_STATEMENTS");

    lua_setfield(L, -2, "CLIENT_MODE"); // cmariadb.CLIENT_MODE[cite: 1]

    return 1;
}