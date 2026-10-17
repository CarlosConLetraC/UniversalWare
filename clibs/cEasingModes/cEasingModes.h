#pragma once
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Quad
int ease_quad_in(lua_State *L);
int ease_quad_out(lua_State *L);
int ease_quad_inout(lua_State *L);

// Cubic
int ease_cubic_in(lua_State *L);
int ease_cubic_out(lua_State *L);
int ease_cubic_inout(lua_State *L);

// Sine
int ease_sine_in(lua_State *L);
int ease_sine_out(lua_State *L);
int ease_sine_inout(lua_State *L);

// Exponential
int ease_exponential_in(lua_State *L);
int ease_exponential_out(lua_State *L);
int ease_exponential_inout(lua_State *L);

// Bounce
int ease_bounce_in(lua_State *L);
int ease_bounce_out(lua_State *L);
int ease_bounce_inout(lua_State *L);