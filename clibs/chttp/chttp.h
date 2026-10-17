#pragma once
#define CHAR_ARRAY_SIZE 1 << 10
#define BUFFER_SIZE 1 << 13

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define CHTTP_MT "CHTTP_Request_Meta"

int luaopen_chttp(lua_State *L);