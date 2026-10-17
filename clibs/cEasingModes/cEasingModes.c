#include "cEasingModes.h"
#include <math.h>

#define PI 3.14159265358979323846f

// --- Quad ---
int ease_quad_in(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t * t);
    return 1;
}
int ease_quad_out(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t * (2.0f - t));
    return 1;
}
int ease_quad_inout(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t < 0.5f ? 2.0f * t * t : 4.0f * t - 2.0f * t * t - 1.0f);
    return 1;
}

// --- Cubic ---
int ease_cubic_in(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t * t * t);
    return 1;
}
int ease_cubic_out(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    float d = t - 1.0f;
    lua_pushnumber(L, d * d * d + 1.0f);
    return 1;
}
int ease_cubic_inout(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    float nt = 2.0f * t - 2.0f;
    lua_pushnumber(L, t < 0.5f ? 4.0f * t * t * t : 0.5f * nt * nt * nt + 1.0f);
    return 1;
}

// --- Sine ---
int ease_sine_in(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, 1.0f - cosf((t * PI) / 2.0f));
    return 1;
}
int ease_sine_out(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, sinf((t * PI) / 2.0f));
    return 1;
}
int ease_sine_inout(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, -(cosf(PI * t) - 1.0f) / 2.0f);
    return 1;
}

// --- Exponential ---
int ease_exponential_in(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t == 0.0f ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f)));
    return 1;
}
int ease_exponential_out(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t));
    return 1;
}
int ease_exponential_inout(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    if (t == 0.0f) { lua_pushnumber(L, 0.0f); return 1; }
    if (t == 1.0f) { lua_pushnumber(L, 1.0f); return 1; }
    if (t < 0.5f) {
        lua_pushnumber(L, powf(2.0f, 20.0f * t - 10.0f) / 2.0f);
    } else {
        lua_pushnumber(L, (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f);
    }
    return 1;
}

// --- Bounce ---
static float bounce_out_formula(float t) {
    float n1 = 121.0f / 16.0f;
    float d1 = 11.0f / 4.0f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) { float t1 = t - 1.5f / d1; return n1 * t1 * t1 + 3.0f / 4.0f; }
    if (t < 2.5f / d1) { float t2 = t - 2.25f / d1; return n1 * t2 * t2 + 15.0f / 16.0f; }
    { float t3 = t - 2.625f / d1; return n1 * t3 * t3 + 63.0f / 64.0f; }
}

int ease_bounce_out(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, bounce_out_formula(t));
    return 1;
}
int ease_bounce_in(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, 1.0f - bounce_out_formula(1.0f - t));
    return 1;
}
int ease_bounce_inout(lua_State *L) {
    float t = (float)luaL_checknumber(L, 1);
    if (t < 0.5f) {
        lua_pushnumber(L, (1.0f - bounce_out_formula(1.0f - (t * 2.0f))) * 0.5f);
    } else {
        lua_pushnumber(L, bounce_out_formula((t * 2.0f) - 1.0f) * 0.5f + 0.5f);
    }
    return 1;
}