#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <raylib.h>

// Declaración de funciones de ventana para exportar al binding principal
int l_init_window(lua_State *L);
int l_window_should_close(lua_State *L);
int l_close_window(lua_State *L);