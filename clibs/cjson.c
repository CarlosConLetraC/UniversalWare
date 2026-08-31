#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

/* Compatibilidad para Lua 5.1 / LuaJIT */
#ifndef lua_rawlen
#define lua_rawlen(L, i) lua_objlen(L, i)
#endif

// Declaraciones adelantadas
static int l_json_encode(lua_State *L);
static int l_json_decode(lua_State *L);

// Función auxiliar recursiva para convertir una tabla/valor de Lua a un json_object *
static struct json_object *lua_to_json_object(lua_State *L, int index) {
    int type = lua_type(L, index);

    switch (type) {
        case LUA_TNIL:
            return NULL;
        case LUA_TBOOLEAN:
            return json_object_new_boolean(lua_toboolean(L, index));
        case LUA_TNUMBER: {
            double num = lua_tonumber(L, index);
            // Comprobar si es un número entero de forma compatible con LuaJIT
            if (num == (long long)num)
                return json_object_new_int64((long long)num);
            else
                return json_object_new_double(num);
        }
        case LUA_TSTRING:
            return json_object_new_string(lua_tostring(L, index));
        case LUA_TTABLE: {
            lua_pushinteger(L, 1);
            lua_gettable(L, index < 0 ? index - 1 : index);
            int is_array = !lua_isnil(L, -1);
            lua_pop(L, 1);

            if (is_array) {
                struct json_object *jarray = json_object_new_array();
                int len = lua_rawlen(L, index);
                for (int i = 1; i <= len; i++) {
                    lua_rawgeti(L, index, i);
                    struct json_object *val = lua_to_json_object(L, -1);
                    if (val)
                        json_object_array_add(jarray, val);
                    else
                        json_object_array_add(jarray, json_object_new_null());
                    lua_pop(L, 1);
                }
                return jarray;
            } else {
                struct json_object *jobj = json_object_new_object();
                lua_pushnil(L);
                while (lua_next(L, index < 0 ? index - 1 : index)) {
                    if (lua_type(L, -2) == LUA_TSTRING) {
                        const char *key = lua_tostring(L, -2);
                        struct json_object *val = lua_to_json_object(L, -1);
                        if (val) json_object_object_add(jobj, key, val);
                    }
                    lua_pop(L, 1);
                }
                return jobj;
            }
        }
        default:
            return NULL;
    }
}

// Función auxiliar recursiva para convertir un json_object * a una tabla de Lua
static void json_object_to_lua(lua_State *L, struct json_object *jobj) {
    if (!jobj) {
        lua_pushnil(L);
        return;
    }

    enum json_type jtype = json_object_get_type(jobj);

    switch (jtype) {
        case json_type_null:
            lua_pushnil(L);
            break;
        case json_type_boolean:
            lua_pushboolean(L, json_object_get_boolean(jobj));
            break;
        case json_type_double:
            lua_pushnumber(L, json_object_get_double(jobj));
            break;
        case json_type_int:
            lua_pushinteger(L, json_object_get_int64(jobj));
            break;
        case json_type_string:
            lua_pushstring(L, json_object_get_string(jobj));
            break;
        case json_type_array: {
            lua_newtable(L);
            int len = json_object_array_length(jobj);
            for (int i = 0; i < len; i++) {
                lua_pushinteger(L, i + 1);
                struct json_object *val = json_object_array_get_idx(jobj, i);
                json_object_to_lua(L, val);
                lua_settable(L, -3);
            }
            break;
        }
        case json_type_object: {
            lua_newtable(L);
            json_object_object_foreach(jobj, key, val) {
                lua_pushstring(L, key);
                json_object_to_lua(L, val);
                lua_settable(L, -3);
            }
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }
}

// Implementación de json.encode(tabla)
static int l_json_encode(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    struct json_object *jobj = lua_to_json_object(L, 1);
    if (!jobj)
        return luaL_error(L, "Error al convertir tabla de Lua a objeto JSON-C");

    const char *json_str = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PLAIN);
    if (!json_str) {
        json_object_put(jobj);
        return luaL_error(L, "Error al serializar JSON");
    }

    lua_pushstring(L, json_str);
    json_object_put(jobj);
    return 1;
}

// Implementación de json.decode(string)
static int l_json_decode(lua_State *L) {
    size_t len;
    const char *json_str = luaL_checklstring(L, 1, &len);

    struct json_object *jobj = json_tokener_parse(json_str);
    if (!jobj)
        return luaL_error(L, "Error de sintaxis JSON (json-c parse falló)");

    json_object_to_lua(L, jobj);
    json_object_put(jobj);
    return 1;
}

// Registro de funciones
static const struct luaL_Reg cjson_funcs[] = {
    {"encode", l_json_encode},
    {"decode", l_json_decode},
    {NULL, NULL}
};

// Función de apertura del módulo para LuaJIT
int luaopen_cjson(lua_State *L) {
    luaL_newlib(L, cjson_funcs);
    return 1;
}