#include "raylib_window.hpp"

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

int l_set_target_fps(lua_State* L) {
    bool lock_with_monitor = lua_toboolean(L, 1);
    int max_fps = 0;
    
    if (!lua_isnoneornil(L, 2)) {
        max_fps = luaL_checkinteger(L, 2);
    }

    int target_fps = 0;

    if (lock_with_monitor) {
        int monitor = GetCurrentMonitor();
        int monitor_refresh = GetMonitorRefreshRate(monitor);
        
        // Si Raylib logra detectar la tasa del monitor, la usamos como base
        if (monitor_refresh > 0) {
            target_fps = monitor_refresh;
        } else {
            target_fps = 60; // Fallback seguro por si el driver no lo reporta
        }
    }

    if (max_fps > 0) {
        if (lock_with_monitor && target_fps > 0) {
            target_fps = (target_fps < max_fps) ? target_fps : max_fps;
        } else {
            target_fps = max_fps;
        }
    }

    SetTargetFPS(target_fps);

    // retornar el valor aplicado para depuración en Lua.
    lua_pushinteger(L, target_fps);
    return 1;
}