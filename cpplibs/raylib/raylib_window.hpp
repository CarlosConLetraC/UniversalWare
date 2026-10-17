#pragma once
extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
    #include <luajit-2.1/lualib.h>
}
#include <raylib.h>

int l_init_window(lua_State *L);
int l_window_should_close(lua_State *L);
int l_close_window(lua_State *L);
int l_set_target_fps(lua_State *L);