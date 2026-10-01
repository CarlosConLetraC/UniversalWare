#include "cColor3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Color3Data* check_color3(lua_State *L, int index) {
    Color3Data *c = (Color3Data*)luaL_checkudata(L, index, "CColor3_MT");
    luaL_argcheck(L, c != NULL, index, "Esperado objeto de tipo Color3.");
    return c;
}

static float clamp_f(float val, float min, float max) {
    // if (val < min) return min;
    // if (val > max) return max;
    // return val;
    return (val < min) ? min :
           (val > max) ? max :
           val;
}

int color3_new(lua_State *L) {
    float r = 0, g = 0, b = 0;
    if (lua_isnumber(L, 1)) r = (float)lua_tonumber(L, 1);
    if (lua_isnumber(L, 2)) g = (float)lua_tonumber(L, 2);
    if (lua_isnumber(L, 3)) b = (float)lua_tonumber(L, 3);

    Color3Data *c = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    c->r = clamp_f(r, 0.0f, 1.0f);
    c->g = clamp_f(g, 0.0f, 1.0f);
    c->b = clamp_f(b, 0.0f, 1.0f);

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_from_rgb(lua_State *L) {
    float r = (float)luaL_optnumber(L, 1, 0.0) / 255.0f;
    float g = (float)luaL_optnumber(L, 2, 0.0) / 255.0f;
    float b = (float)luaL_optnumber(L, 3, 0.0) / 255.0f;

    Color3Data *c = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    c->r = clamp_f(r, 0.0f, 1.0f);
    c->g = clamp_f(g, 0.0f, 1.0f);
    c->b = clamp_f(b, 0.0f, 1.0f);

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_from_hsv(lua_State *L) {
    float h = (float)luaL_optnumber(L, 1, 0.0);
    float s = (float)luaL_optnumber(L, 2, 1.0);
    float v = (float)luaL_optnumber(L, 3, 1.0);

    h = fmodf(h, 360.0f);
    if (h < 0) h += 360.0f;
    s = clamp_f(s, 0.0f, 1.0f);
    v = clamp_f(v, 0.0f, 1.0f);

    float r = 0, g = 0, b = 0;
    if (s == 0.0f) {
        r = g = b = v;
    } else {
        float hh = h / 60.0f;
        int i = (int)floorf(hh);
        float ff = hh - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - (s * ff));
        float t = v * (1.0f - (s * (1.0f - ff)));

        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }

    Color3Data *c = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    c->r = r; c->g = g; c->b = b;

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_from_hex(lua_State *L) {
    const char *hex = luaL_checkstring(L, 1);
    unsigned int rgb_val = 0;
    sscanf(hex, "%x", &rgb_val);

    float r = ((rgb_val >> 16) & 0xFF) / 255.0f;
    float g = ((rgb_val >> 8) & 0xFF) / 255.0f;
    float b = (rgb_val & 0xFF) / 255.0f;

    Color3Data *c = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    c->r = clamp_f(r, 0.0f, 1.0f);
    c->g = clamp_f(g, 0.0f, 1.0f);
    c->b = clamp_f(b, 0.0f, 1.0f);

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_unpack(lua_State *L) {
    Color3Data *c = check_color3(L, 1);
    lua_pushnumber(L, c->r);
    lua_pushnumber(L, c->g);
    lua_pushnumber(L, c->b);
    return 3;
}

int color3_to_hex(lua_State *L) {
    Color3Data *c = check_color3(L, 1);
    int r = (int)floorf(c->r * 255.0f + 0.5f);
    int g = (int)floorf(c->g * 255.0f + 0.5f);
    int b = (int)floorf(c->b * 255.0f + 0.5f);
    
    char buf[7];
    snprintf(buf, sizeof(buf), "%.2X%.2X%.2X", r, g, b);
    lua_pushstring(L, buf);
    return 1;
}

int color3_transform_as_hex(lua_State *L) {
    Color3Data *c = check_color3(L, 1);
    int ir = (int)floorf(c->r * 255.0f + 0.5f);
    int ig = (int)floorf(c->g * 255.0f + 0.5f);
    int ib = (int)floorf(c->b * 255.0f + 0.5f);
    
    lua_pushinteger(L, (ir << 16) | (ig << 8) | ib);
    return 1;
}

int color3_lerp(lua_State *L) {
    Color3Data *c1 = check_color3(L, 1);
    Color3Data *c2 = check_color3(L, 2);
    float t = clamp_f((float)luaL_checknumber(L, 3), 0.0f, 1.0f);

    Color3Data *res = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    res->r = c1->r + (c2->r - c1->r) * t;
    res->g = c1->g + (c2->g - c1->g) * t;
    res->b = c1->b + (c2->b - c1->b) * t;

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_interpolate(lua_State *L) {
    Color3Data *c1 = check_color3(L, 1);
    Color3Data *c2 = check_color3(L, 2);
    float t = (float)luaL_checknumber(L, 3);

    // Si se pasa una función de easing como 4to argumento
    if (lua_isfunction(L, 4)) {
        lua_pushvalue(L, 4);
        lua_pushnumber(L, t);
        lua_call(L, 1, 1);
        t = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    t = clamp_f(t, 0.0f, 1.0f);

    Color3Data *res = (Color3Data*)lua_newuserdata(L, sizeof(Color3Data));
    res->r = c1->r + (c2->r - c1->r) * t;
    res->g = c1->g + (c2->g - c1->g) * t;
    res->b = c1->b + (c2->b - c1->b) * t;

    luaL_getmetatable(L, "CColor3_MT");
    lua_setmetatable(L, -2);
    return 1;
}

int color3_tostring(lua_State *L) {
    Color3Data *c = check_color3(L, 1);
    char buf[64];
    snprintf(buf, sizeof(buf), "Color3(%.12g, %.12g, %.12g)", c->r, c->g, c->b);
    lua_pushstring(L, buf);
    return 1;
}

int color3_eq(lua_State *L) {
    Color3Data *c1 = check_color3(L, 1);
    Color3Data *c2 = check_color3(L, 2);
    lua_pushboolean(L, (c1->r == c2->r) && (c1->g == c2->g) && (c1->b == c2->b));
    return 1;
}