#pragma once
#define CHAR_ARRAY_SIZE 1024

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define CHTTP_MT "CHTTP_Request_Meta"

int luaopen_chttp(lua_State *L);