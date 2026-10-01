#include "cEasingModes.h"

int luaopen_cEasingModes(lua_State *L) {
    lua_newtable(L);

    #define REGISTER_STYLE(name, in_fn, out_fn, inout_fn) \
        lua_newtable(L); \
        lua_pushcfunction(L, in_fn); lua_setfield(L, -2, "In"); \
        lua_pushcfunction(L, out_fn); lua_setfield(L, -2, "Out"); \
        lua_pushcfunction(L, inout_fn); lua_setfield(L, -2, "InOut"); \
        lua_setfield(L, -2, name);

    REGISTER_STYLE("Quad", ease_quad_in, ease_quad_out, ease_quad_inout);
    REGISTER_STYLE("Cubic", ease_cubic_in, ease_cubic_out, ease_cubic_inout);
    REGISTER_STYLE("Sine", ease_sine_in, ease_sine_out, ease_sine_inout);
    REGISTER_STYLE("Exponential", ease_exponential_in, ease_exponential_out, ease_exponential_inout);
    REGISTER_STYLE("Bounce", ease_bounce_in, ease_bounce_out, ease_bounce_inout);

    #undef REGISTER_STYLE

    return 1;
}