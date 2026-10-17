#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct {
    float r;
    float g;
    float b;
} Color3Data;

Color3Data* check_color3(lua_State *L, int index);