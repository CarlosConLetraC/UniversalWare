#include "raylib_draw.hpp"

// Definición de la estructura Color3Data compatible con cColor3
typedef struct {
    float r;
    float g;
    float b;
} Color3Data;

// Función auxiliar para extraer un Color de Raylib desde Lua
// Soporta:
// 1. Userdata de cColor3 (metatable "CColor3_MT")
// 2. Tabla Lua {r, g, b, a} o {r, g, b}
// 3. Argumentos numéricos directos r, g, b, a
static Color lua_to_color(lua_State *L, int index) {
    Color col = { 255, 255, 255, 255 }; // Blanco por defecto

    // 1. Verificar si es un objeto cColor3
    if (lua_isuserdata(L, index)) {
        Color3Data *c3 = (Color3Data*)luaL_testudata(L, index, "CColor3_MT");
        if (c3 != NULL) {
            // cColor3 usa floats de 0.0 a 1.0; Raylib usa bytes de 0 a 255
            col.r = (unsigned char)(c3->r * 255.0f + 0.5f);
            col.g = (unsigned char)(c3->g * 255.0f + 0.5f);
            col.b = (unsigned char)(c3->b * 255.0f + 0.5f);
            col.a = 255; // Opacidad completa por defecto
            return col;
        }
    }

    // 2. Verificar si es una tabla Lua
    if (lua_istable(L, index)) {
        lua_rawgeti(L, index, 1); col.r = (unsigned char)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, index, 2); col.g = (unsigned char)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, index, 3); col.b = (unsigned char)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, index, 4); col.a = (unsigned char)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
    } 
    // 3. Verificar si son números independientes
    else if (lua_isnumber(L, index)) {
        col.r = (unsigned char)luaL_checkinteger(L, index + 0);
        col.g = (unsigned char)luaL_checkinteger(L, index + 1);
        col.b = (unsigned char)luaL_checkinteger(L, index + 2);
        col.a = (unsigned char)luaL_optinteger(L, index + 3, 255);
    }

    return col;
}

int l_begin_drawing(lua_State *L) {
    BeginDrawing();
    return 0;
}

int l_end_drawing(lua_State *L) {
    EndDrawing();
    return 0;
}

int l_clear_background(lua_State *L) {
    Color bg = lua_to_color(L, 1);
    ClearBackground(bg);
    return 0;
}

int l_draw_text(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    int fontSize = (int)luaL_checkinteger(L, 4);
    
    // Soporta pasar un objeto cColor3 directamente como 5to argumento
    Color tint = lua_to_color(L, 5);

    DrawText(text, posX, posY, fontSize, tint);
    return 0;
}