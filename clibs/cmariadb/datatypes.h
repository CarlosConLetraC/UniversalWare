#ifndef CMARIADB_DATATYPES_H
#define CMARIADB_DATYPES_H

#include <lua.h>
#include <lauxlib.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>

#define SQLVALUE_META "SqlValueMeta"

// Estructura contenedora para el Userdata de Lua
typedef struct {
    MYSQL *conn;
} LuaMariaDB;

typedef struct {
    enum enum_field_types sql_type; // Identificador exacto de MariaDB
    
    // Unión para albergar el valor nativo según corresponda
    union {
        int64_t int_val;        // TINYINT, SMALLINT, INT, BIGINT, BOOLEAN
        double float_val;       // FLOAT, DOUBLE, DECIMAL
        struct {
            char *ptr;          // Puntero dinámico para VARCHAR, TEXT, BLOB grandes
            size_t len;         // Longitud exacta de los datos
        } string_val;
    } data;
    
    int is_null;                // Bandera para valores NULL de SQL
} SqlValue;

SqlValue* l_sqlvalue_create_impl(lua_State *L, enum enum_field_types type, int is_int);
SqlValue* new_sql_value(lua_State *L, enum enum_field_types type);

#define l_sqlvalue_float(L)   (l_sqlvalue_create_impl((L), MYSQL_TYPE_FLOAT, 0))
#define l_sqlvalue_double(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_DOUBLE, 0))
#define l_sqlvalue_decimal(L) (l_sqlvalue_create_impl((L), MYSQL_TYPE_NEWDECIMAL, 0))
#define l_sqlvalue_integer(L) (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONG, 1))
#define l_sqlvalue_year(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_YEAR, 1))
#define l_sqlvalue_string(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_VAR_STRING, 0))
#define l_sqlvalue_bigint(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONGLONG, 1))
#define l_sqlvalue_tiny(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_TINY, 1))
#define l_sqlvalue_small(L)   (l_sqlvalue_create_impl((L), MYSQL_TYPE_SHORT, 1))
#define l_sqlvalue_medium(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_INT24, 1))
#define l_sqlvalue_date(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_DATE, 0))
#define l_sqlvalue_datetime(L)(l_sqlvalue_create_impl((L), MYSQL_TYPE_DATETIME, 0))
#define l_sqlvalue_time(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_TIME, 0))
#define l_sqlvalue_blob(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONG_BLOB, 0))

void register_sqlvalue_meta(lua_State *L);
void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length);

// Wrappers expuestos hacia Lua
int wrap_sqlvalue_float(lua_State *L);
int wrap_sqlvalue_double(lua_State *L);
int wrap_sqlvalue_decimal(lua_State *L);
int wrap_sqlvalue_integer(lua_State *L);
int wrap_sqlvalue_bigint(lua_State *L);
int wrap_sqlvalue_tiny(lua_State *L);
int wrap_sqlvalue_small(lua_State *L);
int wrap_sqlvalue_medium(lua_State *L);
int wrap_sqlvalue_year(lua_State *L);
int wrap_sqlvalue_string(lua_State *L);
int wrap_sqlvalue_date(lua_State *L);
int wrap_sqlvalue_datetime(lua_State *L);
int wrap_sqlvalue_time(lua_State *L);
int wrap_sqlvalue_blob(lua_State *L);

#endif