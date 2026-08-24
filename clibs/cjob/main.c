#include "cjob.h"

Job *job_head = NULL;
Job *job_tail = NULL;

static const struct luaL_Reg job_methods[] = {
    {"kill",       l_job_kill},
    {"stop",       l_job_stop},
    {"resume",     l_job_resume},
    {"__index",    l_job_index},
    {"__tostring", l_job_tostring},
    {"__gc",       l_job_gc},
    {NULL, NULL}
};

static const struct luaL_Reg cjob_funcs[] = {
    {"new",   l_cjob_new},
    {"wait",  l_cjob_wait},
    {"async", l_cjob_async},
    {NULL, NULL}
};

int luaopen_cjob(lua_State *L) {
    luaL_newmetatable(L, CJOB_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_register(L, NULL, job_methods);

    // Sentinel para iniciar el despacho automatico al terminar el chunk principal
    luaL_newmetatable(L, CJOB_SENTINEL_MT);
    lua_pushcfunction(L, l_sentinel_gc);
    lua_setfield(L, -2, "__gc");

    lua_newuserdata(L, sizeof(int));
    luaL_getmetatable(L, CJOB_SENTINEL_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, LUA_REGISTRYINDEX, "_CJOB_SENTINEL_");
    luaL_register(L, "cjob", cjob_funcs);
    return 1;
}