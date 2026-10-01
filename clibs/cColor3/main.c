#include "cColor3.h"

int color3_new(lua_State *L);
int color3_from_rgb(lua_State *L);
int color3_from_hsv(lua_State *L);
int color3_from_hex(lua_State *L);
int color3_unpack(lua_State *L);
int color3_to_hex(lua_State *L);
int color3_transform_as_hex(lua_State *L);
int color3_lerp(lua_State *L);
int color3_interpolate(lua_State *L);
int color3_tostring(lua_State *L);
int color3_eq(lua_State *L);

static const struct luaL_Reg color3_methods[] = {
    {"unpack", color3_unpack},
    {"toHEX", color3_to_hex},
    {"transformAsHEX", color3_transform_as_hex},
    {"lerp", color3_lerp},
    {"interpolate", color3_interpolate},
    {NULL, NULL}
};

static const struct luaL_Reg color3_functions[] = {
    {"new", color3_new},
    {"fromRGB", color3_from_rgb},
    {"fromHSV", color3_from_hsv},
    {"fromHEX", color3_from_hex},
    {NULL, NULL}
};

int luaopen_cColor3(lua_State *L) {
    luaL_newmetatable(L, "CColor3_MT");
    
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    
    lua_pushcfunction(L, color3_tostring);
    lua_setfield(L, -2, "__tostring");
    
    lua_pushcfunction(L, color3_eq);
    lua_setfield(L, -2, "__eq");

    luaL_setfuncs(L, color3_methods, 0);

    luaL_newlib(L, color3_functions);
    return 1;
}