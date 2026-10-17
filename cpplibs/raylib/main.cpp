extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
    #include <luajit-2.1/lualib.h>
}

#include "raylib_window.h"
#include "raylib_draw.h"

static const struct luaL_Reg raylib_funcs[] = {
    // Ventana
    {"init_window", l_init_window},
    {"window_should_close", l_window_should_close},
    {"close_window", l_close_window},
    
    // Dibujo
    {"begin_drawing", l_begin_drawing},
    {"end_drawing", l_end_drawing},
    {"clear_background", l_clear_background},
    
    {NULL, NULL}
};

extern "C" int luaopen_raylib(lua_State *L) {
    luaL_newlib(L, raylib_funcs);
    return 1;
}