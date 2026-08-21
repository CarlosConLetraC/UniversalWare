#ifndef CMARIADB_DATATYPES_H
#define CMARIADB_DATATYPES_H

#include <lua.h>
#include <lauxlib.h>
//#include <mysql.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>

#define SQLVALUE_META "SqlValueMeta"

// Estructura que preserva el valor SQL exacto y sus metadatos
typedef struct {
    enum enum_field_types sql_type; // Tipo original de MariaDB/MySQL
    double num_val;                 // Valor numérico (double)
    char str_repr[64];              // Representación en texto exacto
} SqlValue;

// Prototipos de funciones expuestas para cmariadb.c
void register_sqlvalue_meta(lua_State *L);
void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length);

#endif // CMARIADB_DATATYPES_H