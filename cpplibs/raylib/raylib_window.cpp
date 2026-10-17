#include "raylib_window.h"

int l_init_window(lua_State *L) {
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    const char *title = luaL_checkstring(L, 3);
    InitWindow(width, height, title);
    return 0;
}

int l_window_should_close(lua_State *L) {
    lua_pushboolean(L, WindowShouldClose());
    return 1;
}

int l_close_window(lua_State *L) {
    CloseWindow();
    return 0;
}