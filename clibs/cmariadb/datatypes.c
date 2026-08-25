#include "datatypes.h"

// 1. Destructor / Recolector de basura (Liberación segura de memoria dinámica y blobs/geometría)
static int sqlvalue_gc(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    if (!v->is_null) {
        switch (v->sql_type) {
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_JSON:
            case MYSQL_TYPE_ENUM:
            case MYSQL_TYPE_SET:
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
                if (v->data.string_val.ptr) {
                    free(v->data.string_val.ptr);
                    v->data.string_val.ptr = NULL;
                }
                break;
            case MYSQL_TYPE_GEOMETRY:
                if (v->data.blob_val.ptr) {
                    free((void *)v->data.blob_val.ptr);
                    v->data.blob_val.ptr = NULL;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

// 2. Metamétodo __tostring (Devuelve cadenas formateadas para la consulta SQL o representación)
static int sqlvalue_tostring(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    if (v->is_null) {
        lua_pushstring(L, "NULL");
        return 1;
    }

    switch (v->sql_type) {
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%lld", (long long)v->data.int_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_BOOL: {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", (int)v->data.int_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_DECIMAL: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%f", v->data.float_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_GEOMETRY: {
            if (v->data.blob_val.ptr)
                lua_pushlstring(L, (const char *)v->data.blob_val.ptr, v->data.blob_val.len);
            else
                lua_pushstring(L, "");
            break;
        }
        default: {
            if (v->data.string_val.ptr)
                lua_pushlstring(L, v->data.string_val.ptr, v->data.string_val.len);
            else
                lua_pushstring(L, "");
            break;
        }
    }
    return 1;
}

// 3. Método tonumber()
static int sqlvalue_tonumber(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    if (v->is_null) {
        lua_pushnil(L);
        return 1;
    }
    switch (v->sql_type) {
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_BOOL:
            lua_pushnumber(L, (lua_Number)v->data.int_val);
            break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_DECIMAL:
            lua_pushnumber(L, v->data.float_val);
            break;
        case MYSQL_TYPE_GEOMETRY:
            lua_pushnumber(L, 0.0);
            break;
        default:
            lua_pushnumber(L, v->data.string_val.ptr ? atof(v->data.string_val.ptr) : 0.0);
            break;
    }
    return 1;
}

// 4. Método type() (Mapeo completo de los tipos de MariaDB)
static int sqlvalue_type(lua_State *L) {
    SqlValue *v = (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    switch (v->sql_type) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            lua_pushstring(L, "DECIMAL");
            break;
        case MYSQL_TYPE_TINY:
            lua_pushstring(L, "TINYINT");
            break;
        case MYSQL_TYPE_SHORT:
            lua_pushstring(L, "SMALLINT");
            break;
        case MYSQL_TYPE_INT24:
            lua_pushstring(L, "MEDIUMINT");
            break;
        case MYSQL_TYPE_LONG:
            lua_pushstring(L, "INTEGER");
            break;
        case MYSQL_TYPE_LONGLONG:
            lua_pushstring(L, "BIGINT");
            break;
        case MYSQL_TYPE_YEAR:
            lua_pushstring(L, "YEAR");
            break;
        case MYSQL_TYPE_BIT:
            lua_pushstring(L, "BIT");
            break;
        case MYSQL_TYPE_BOOL:
            lua_pushstring(L, "BOOLEAN");
            break;
        case MYSQL_TYPE_FLOAT:
            lua_pushstring(L, "FLOAT");
            break;
        case MYSQL_TYPE_DOUBLE:
            lua_pushstring(L, "DOUBLE");
            break;
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_STRING:
            lua_pushstring(L, "STRING");
            break;
        case MYSQL_TYPE_JSON:
            lua_pushstring(L, "JSON");
            break;
        case MYSQL_TYPE_ENUM:
            lua_pushstring(L, "ENUM");
            break;
        case MYSQL_TYPE_SET:
            lua_pushstring(L, "SET");
            break;
        case MYSQL_TYPE_DATE:
            lua_pushstring(L, "DATE");
            break;
        case MYSQL_TYPE_DATETIME:
            lua_pushstring(L, "DATETIME");
            break;
        case MYSQL_TYPE_TIME:
            lua_pushstring(L, "TIME");
            break;
        case MYSQL_TYPE_TIMESTAMP:
            lua_pushstring(L, "TIMESTAMP");
            break;
        case MYSQL_TYPE_GEOMETRY:
            lua_pushstring(L, "GEOMETRY");
            break;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            lua_pushstring(L, "BLOB");
            break;
        default:
            lua_pushstring(L, "UNKNOWN");
            break;
    }
    return 1;
}

// 5. Registro de la metatabla y sus métodos de instancia
void register_sqlvalue_meta(lua_State *L) {
    luaL_newmetatable(L, SQLVALUE_META);

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, sqlvalue_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, sqlvalue_tonumber);
    lua_setfield(L, -2, "tonumber");

    lua_pushcfunction(L, sqlvalue_type);
    lua_setfield(L, -2, "type");

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "tostring");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);
}

// 6. Función base constructora robusta ampliada
SqlValue* l_sqlvalue_create_impl(lua_State *L, enum enum_field_types type, int is_int) {
    int has_arg = (lua_gettop(L) >= 1 && !lua_isnil(L, 1));

    SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
    v->sql_type = type;
    v->is_null = !has_arg ? 1 : 0;
    memset(&v->data, 0, sizeof(v->data));

    if (!v->is_null) {
        if (is_int || type == MYSQL_TYPE_YEAR || type == MYSQL_TYPE_BIT || type == MYSQL_TYPE_BOOL) {
            if (lua_type(L, 1) == 10) { // LUA_TCDATA de LuaJIT
                int64_t *pval = (int64_t *)lua_topointer(L, 1);
                v->data.int_val = pval ? *pval : 0;
            } else {
                v->data.int_val = (int64_t)lua_tonumber(L, 1);
            }
        } 
        else if (type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_DOUBLE || type == MYSQL_TYPE_NEWDECIMAL || type == MYSQL_TYPE_DECIMAL) {
            v->data.float_val = (double)lua_tonumber(L, 1);
        } 
        else if (type == MYSQL_TYPE_GEOMETRY) {
            size_t len = 0;
            const char *str = lua_tolstring(L, 1, &len);
            if (str) {
                unsigned char *buf = (unsigned char *)malloc(len);
                if (buf) {
                    memcpy(buf, str, len);
                    v->data.blob_val.ptr = buf;
                    v->data.blob_val.len = len;
                }
            }
        }
        else {
            size_t len = 0;
            const char *str = lua_tolstring(L, 1, &len);
            if (str) {
                v->data.string_val.ptr = (char *)malloc(len + 1);
                if (v->data.string_val.ptr) {
                    memcpy(v->data.string_val.ptr, str, len);
                    v->data.string_val.ptr[len] = '\0';
                    v->data.string_val.len = len;
                }
            } else {
                v->data.string_val.ptr = strdup("");
                v->data.string_val.len = 0;
            }
        }
    }

    luaL_getmetatable(L, SQLVALUE_META);
    lua_setmetatable(L, -2);
    return v;
}

// 7. Mapeador de campos recibidos desde MariaDB (Lectura con cobertura total)
void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length) {
    SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
    v->sql_type = field->type;
    memset(&v->data, 0, sizeof(v->data));

    if (!val) {
        v->is_null = 1;
    } else {
        v->is_null = 0;
        switch (field->type) {
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_SHORT:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONGLONG:
            case MYSQL_TYPE_YEAR:
            case MYSQL_TYPE_BIT:
            case MYSQL_TYPE_BOOL:
                v->data.int_val = strtoll(val, NULL, 10);
                break;

            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                v->data.float_val = strtod(val, NULL);
                break;

            case MYSQL_TYPE_GEOMETRY:
                v->data.blob_val.ptr = (unsigned char *)malloc(length);
                if (v->data.blob_val.ptr) {
                    memcpy((void *)v->data.blob_val.ptr, val, length);
                    v->data.blob_val.len = length;
                } else {
                    v->data.blob_val.len = 0;
                }
                break;

            default:
                v->data.string_val.ptr = (char *)malloc(length + 1);
                if (v->data.string_val.ptr) {
                    memcpy(v->data.string_val.ptr, val, length);
                    v->data.string_val.ptr[length] = '\0';
                    v->data.string_val.len = length;
                } else {
                    v->data.string_val.len = 0;
                }
                break;
        }
    }

    luaL_getmetatable(L, SQLVALUE_META);
    lua_setmetatable(L, -2);
}

// 8. Wrappers expuestos a Lua ampliados
int wrap_sqlvalue_tiny(lua_State *L)     { l_sqlvalue_tiny(L); return 1; }
int wrap_sqlvalue_small(lua_State *L)    { l_sqlvalue_small(L); return 1; }
int wrap_sqlvalue_medium(lua_State *L)   { l_sqlvalue_medium(L); return 1; }
int wrap_sqlvalue_integer(lua_State *L)  { l_sqlvalue_integer(L); return 1; }
int wrap_sqlvalue_bigint(lua_State *L)   { l_sqlvalue_bigint(L); return 1; }
int wrap_sqlvalue_year(lua_State *L)     { l_sqlvalue_year(L); return 1; }
int wrap_sqlvalue_bit(lua_State *L)      { l_sqlvalue_bit(L); return 1; }
int wrap_sqlvalue_bool(lua_State *L)     { l_sqlvalue_bool(L); return 1; }

int wrap_sqlvalue_float(lua_State *L)    { l_sqlvalue_float(L); return 1; }
int wrap_sqlvalue_double(lua_State *L)   { l_sqlvalue_double(L); return 1; }
int wrap_sqlvalue_decimal(lua_State *L)  { l_sqlvalue_decimal(L); return 1; }

int wrap_sqlvalue_string(lua_State *L)   { l_sqlvalue_string(L); return 1; }
int wrap_sqlvalue_json(lua_State *L)     { l_sqlvalue_json(L); return 1; }
int wrap_sqlvalue_enum(lua_State *L)     { l_sqlvalue_enum(L); return 1; }
int wrap_sqlvalue_set(lua_State *L)      { l_sqlvalue_set(L); return 1; }

int wrap_sqlvalue_date(lua_State *L)     { l_sqlvalue_date(L); return 1; }
int wrap_sqlvalue_datetime(lua_State *L) { l_sqlvalue_datetime(L); return 1; }
int wrap_sqlvalue_time(lua_State *L)     { l_sqlvalue_time(L); return 1; }
int wrap_sqlvalue_timestamp(lua_State *L){ l_sqlvalue_timestamp(L); return 1; }

int wrap_sqlvalue_blob(lua_State *L)     { l_sqlvalue_blob(L); return 1; }
int wrap_sqlvalue_geometry(lua_State *L) { l_sqlvalue_geometry(L); return 1; }