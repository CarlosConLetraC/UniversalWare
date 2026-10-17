#pragma once
extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
}
#include <raylib.h>

// Declaraciones de funciones expuestas a LuaJIT
int l_begin_drawing(lua_State *L);
int l_end_drawing(lua_State *L);
int l_clear_background(lua_State *L);
int l_draw_text(lua_State *L);