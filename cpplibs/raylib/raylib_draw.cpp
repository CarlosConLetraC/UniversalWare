#include "raylib_draw.h"

int l_begin_drawing(lua_State *L) {
    BeginDrawing();
    return 0;
}

int l_end_drawing(lua_State *L) {
    EndDrawing();
    return 0;
}

int l_clear_background(lua_State *L) {
    // Podrías extenderlo para recibir colores desde Lua, por ahora usamos un blanco por defecto
    ClearBackground(RAYWHITE);
    return 0;
}