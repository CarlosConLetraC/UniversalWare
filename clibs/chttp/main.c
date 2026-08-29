#include "chttp.h"

// Declaradas en server.c
extern int l_chttp_listen(lua_State *L);
extern int l_chttp_accept(lua_State *L);
extern int l_request_index(lua_State *L);
extern int l_request_gc(lua_State *L);

static const struct luaL_Reg chttp_methods[] = {
    {"__index", l_request_index},
//    {"__gc", l_request_gc},
    {NULL, NULL}
};

static const struct luaL_Reg chttp_funcs[] = {
    {"listen", l_chttp_listen},
    {"accept", l_chttp_accept},
    {NULL, NULL}
};

int luaopen_chttp(lua_State *L) {
    luaL_newmetatable(L, CHTTP_MT);
    lua_pushcfunction(L, l_request_gc);
    lua_setfield(L, -2, "__gc");
    
    luaL_register(L, NULL, chttp_methods);

    luaL_register(L, "chttp", chttp_funcs);
    return 1;
}