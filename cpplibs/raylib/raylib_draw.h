#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <raylib.h>

int l_begin_drawing(lua_State *L);
int l_end_drawing(lua_State *L);
int l_clear_background(lua_State *L);