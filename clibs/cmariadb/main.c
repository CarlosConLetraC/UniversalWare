#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql/mysql.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "datatypes.h"
#include "clientmodes.h"

// 1. Conexion a MariaDB
static int l_connect(lua_State *L) {
    const char *host = NULL,
               *user = NULL,
               *pass = NULL,
               *db   = NULL;
    int port = 3306;
    unsigned long client_flags = MARIADB_CLIENT_DEFAULT_MODE;
    const char *socket_path = NULL;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "host");
        if (lua_isstring(L, -1)) host = lua_tostring(L, -1);
        
        lua_pop(L, 1);
        lua_getfield(L, 1, "user");
        if (lua_isstring(L, -1)) user = lua_tostring(L, -1);
        
        lua_pop(L, 1);
        lua_getfield(L, 1, "password");
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            lua_getfield(L, 1, "pass");
        }
        if (lua_isstring(L, -1)) pass = lua_tostring(L, -1);
        
        lua_pop(L, 1);
        if (!host || !user || !pass)
            return luaL_error(L, "La tabla de conexion requiere las claves 'host', 'user' y 'password'");

        lua_getfield(L, 1, "db");
        if (lua_isstring(L, -1)) db = lua_tostring(L, -1);
        
        lua_pop(L, 1);
        lua_getfield(L, 1, "port");
        if (lua_isnumber(L, -1)) port = (int)lua_tointeger(L, -1);
        
        lua_pop(L, 1);
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
        lua_getfield(L, 1, "socket"); if (lua_isstring(L, -1)) socket_path = lua_tostring(L, -1); lua_pop(L, 1);
    } else {
        host = luaL_checkstring(L, 1);
        user = luaL_checkstring(L, 2);
        pass = luaL_checkstring(L, 3);
        db   = luaL_optstring(L, 4, NULL);
        port = (int)luaL_optinteger(L, 5, 3306);

        if (lua_gettop(L) >= 6 && !lua_isnil(L, 6)) {
            int custom_mode = (int)luaL_checkinteger(L, 6);
            if (custom_mode == MARIADB_CLIENT_DEFAULT_MODE || custom_mode == MARIADB_CLIENT_MULTI_STATEMENTS)
                client_flags = (unsigned long)custom_mode;
            else
                return luaL_error(L, "client_mode no permitido: %d", custom_mode);
        }
        socket_path = luaL_optstring(L, 7, NULL);
    }

    if (db && strlen(db) == 0) db = NULL;
    if (socket_path && strlen(socket_path) == 0) socket_path = NULL;

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

// 2. Consulta estandar
static int l_query(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    const char *sql = luaL_checkstring(L, 2);

    if (!db_obj->conn) return luaL_error(L, "La conexion a la base de datos esta cerrada");

    if (mysql_query(db_obj->conn, sql) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, mysql_error(db_obj->conn));
        return 2;
    }

    MYSQL_RES *res = mysql_store_result(db_obj->conn);
    if (!res) {
        lua_pushboolean(L, 1);
        return 1;
    }

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
    return 1;
}

// 3. Consultas multiples (multi_query)
static int l_multi_query(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    const char *sql_script = luaL_checkstring(L, 2);

    if (!db_obj->conn) return luaL_error(L, "La conexion a la base de datos esta cerrada");

    lua_newtable(L);
    int statement_index = 1;
    const char *p = sql_script;
    const char *stmt_start = p;
    char in_quote = 0;

    while (*p != '\0') {
        char c = *p;
        if (c == '\'' || c == '"' || c == '`') {
            if (in_quote == 0) in_quote = c;
            else if (in_quote == c && *(p - 1) != '\\') in_quote = 0;
        } else if (c == ';' && in_quote == 0) {
            size_t len = p - stmt_start;
            if (len > 0) {
                char *single_stmt = (char *)malloc(len + 1);
                memcpy(single_stmt, stmt_start, len);
                single_stmt[len] = '\0';

                char *p_trim = single_stmt;
                // while (*p_trim == '\32' || *p_trim == '\t' || *p_trim == '\n' || *p_trim == '\r') p_trim++;
                for (; *p_trim == '\32' || *p_trim == '\t' || *p_trim == '\n' || *p_trim == '\r'; p_trim++);

                if (*p_trim != '\0') {
                    if (mysql_query(db_obj->conn, single_stmt) != 0) {
                        lua_pushnil(L);
                        lua_pushfstring(L, "Error en sentencia #%d: %s", statement_index, mysql_error(db_obj->conn));
                        free(single_stmt);
                        return 2;
                    }

                    MYSQL_RES *res = mysql_store_result(db_obj->conn);
                    if (res) {
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
            stmt_start = p + 1;
        }
        p++;
    }
    return 1;
}

// 4. Cerrar conexion
static int l_close(lua_State *L) {
    LuaMariaDB *db_obj = (LuaMariaDB *)luaL_checkudata(L, 1, MARIADB_LUA_METATABLE);
    if (db_obj->conn) {
        mysql_close(db_obj->conn);
        db_obj->conn = NULL;
    }
    return 0;
}

static const struct luaL_Reg db_methods[] = {
    {"query",       l_query},
    {"multi_query", l_multi_query},
    {"close",       l_close},
    {"__gc",        l_close},
    {NULL, NULL}
};

// 5. Punto de entrada principal ampliado con cobertura total de tipos
int luaopen_cmariadb(lua_State *L) {
    register_sqlvalue_meta(L);

    if (luaL_newmetatable(L, MARIADB_LUA_METATABLE)) {
        luaL_setfuncs(L, db_methods, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);

    luaL_Reg functions[] = {
        {"connect", l_connect},
        {NULL, NULL}
    };
    luaL_newlib(L, functions);

    // CLIENT_MODE
    lua_newtable(L);
    lua_pushinteger(L, MARIADB_CLIENT_DEFAULT_MODE); lua_setfield(L, -2, "DEFAULT");
    lua_pushinteger(L, MARIADB_CLIENT_MULTI_STATEMENTS); lua_setfield(L, -2, "MULTI_STATEMENTS");
    lua_setfield(L, -2, "CLIENT_MODE");

    // SqlValue factory subtable (Ampliacion completa)
    lua_newtable(L);
    // Numericos
    lua_pushcfunction(L, wrap_sqlvalue_tiny);     lua_setfield(L, -2, "tinyint");
    lua_pushcfunction(L, wrap_sqlvalue_small);    lua_setfield(L, -2, "smallint");
    lua_pushcfunction(L, wrap_sqlvalue_medium);   lua_setfield(L, -2, "mediumint");
    lua_pushcfunction(L, wrap_sqlvalue_integer);  lua_setfield(L, -2, "integer");
    lua_pushcfunction(L, wrap_sqlvalue_bigint);   lua_setfield(L, -2, "bigint");
    lua_pushcfunction(L, wrap_sqlvalue_year);     lua_setfield(L, -2, "year");
    lua_pushcfunction(L, wrap_sqlvalue_bit);      lua_setfield(L, -2, "bit");
    lua_pushcfunction(L, wrap_sqlvalue_bool);     lua_setfield(L, -2, "boolean");

    // Decimales y Flotantes
    lua_pushcfunction(L, wrap_sqlvalue_float);    lua_setfield(L, -2, "float");
    lua_pushcfunction(L, wrap_sqlvalue_double);   lua_setfield(L, -2, "double");
    lua_pushcfunction(L, wrap_sqlvalue_decimal);  lua_setfield(L, -2, "decimal");

    // Cadenas, Textos y Estructuras Especiales
    lua_pushcfunction(L, wrap_sqlvalue_string);   lua_setfield(L, -2, "string");
    lua_pushcfunction(L, wrap_sqlvalue_json);     lua_setfield(L, -2, "json");
    lua_pushcfunction(L, wrap_sqlvalue_enum);     lua_setfield(L, -2, "enum");
    lua_pushcfunction(L, wrap_sqlvalue_set);      lua_setfield(L, -2, "set");

    // Fechas y Horas Temporales
    lua_pushcfunction(L, wrap_sqlvalue_date);     lua_setfield(L, -2, "date");
    lua_pushcfunction(L, wrap_sqlvalue_datetime); lua_setfield(L, -2, "datetime");
    lua_pushcfunction(L, wrap_sqlvalue_time);     lua_setfield(L, -2, "time");
    lua_pushcfunction(L, wrap_sqlvalue_timestamp);lua_setfield(L, -2, "timestamp");

    // Binarios, Blobs y Geometria Espacial
    lua_pushcfunction(L, wrap_sqlvalue_blob);     lua_setfield(L, -2, "blob");
    lua_pushcfunction(L, wrap_sqlvalue_geometry); lua_setfield(L, -2, "geometry");

    lua_setfield(L, -2, "SqlValue");

    return 1;
}