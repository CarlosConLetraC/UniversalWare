#include "cjob.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <luajit.h>

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

static void cjob_line_hook(lua_State *L, lua_Debug *ar) {
    (void)ar; // Forzamos un yield de 0 segundos (interrupción inmediata por quantum)
    if (lua_isyieldable(L))
        lua_yield(L, 0);
}

int l_cjob_new(lua_State *L) {
    lua_State *co = NULL;
    int co_ref = LUA_NOREF;

    if (lua_isfunction(L, 1)) {
        co = lua_newthread(L);
        co_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_pushvalue(L, 1);
        lua_xmove(L, co, 1);
    } else {
        return luaL_error(L, "Se esperaba una funcion");
    }

    // luaJIT_setmode(co, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);

    Job *j = (Job *)malloc(sizeof(Job));
    j->co = co;
    j->co_ref = co_ref;
    j->status = JOB_RUNNING;
    j->wake_at = 0.0;
    j->next = NULL;

    // ACTIVAR HOOK: MASKCOUNT = 1 dispara el hook en CADA OpCode de Lua
    lua_sethook(co, cjob_line_hook, LUA_MASKCOUNT, 1);

    enqueue_job(j);

    // Devolver el Handle a Lua
    JobHandle *handle = (JobHandle *)lua_newuserdata(L, sizeof(JobHandle));
    handle->job = j;
    luaL_getmetatable(L, CJOB_MT);
    lua_setmetatable(L, -2);

    return 1;
}

int l_job_kill(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (h && h->job) {
        h->job->status = JOB_DEAD;
        if (h->job->co_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, h->job->co_ref);
            h->job->co_ref = LUA_NOREF;
        }
    }
    return 0;
}

int l_job_stop(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (h && h->job && h->job->status == JOB_RUNNING) {
        h->job->status = JOB_SUSPENDED;
    }
    return 0;
}

int l_job_resume(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (h && h->job && h->job->status == JOB_SUSPENDED) {
        h->job->status = JOB_RUNNING;
        h->job->wake_at = 0.0;
        enqueue_job(h->job);
    }
    return 0;
}

int l_job_index(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    const char *key = luaL_checkstring(L, 2);

    if (strcmp(key, "status") == 0) {
        if (!h->job || h->job->status == JOB_DEAD) {
            lua_pushstring(L, "dead");
        } else if (h->job->status == JOB_SUSPENDED) {
            lua_pushstring(L, "suspended");
        } else {
            lua_pushstring(L, "running");
        }
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    return 1;
}

int l_job_gc(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (h && h->job) {
        // Desanclar referencia del registry si el job se destruye en Lua
        if (h->job->co_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, h->job->co_ref);
            h->job->co_ref = LUA_NOREF;
        }
        // Desvincular puntero para evitar accesos "dangling"
        h->job->status = JOB_DEAD;
        free(h->job);
        h->job = NULL;
    }
    return 0;
}

int l_job_tostring(lua_State *L) {
    JobHandle *h = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    
    if (!h || !h->job) {
        lua_pushfstring(L, "<job[dead]: %p>", h);
        return 1;
    }

    const char *status_str = "running";
    if (h->job->status == JOB_SUSPENDED) {
        status_str = "suspended";
    } else if (h->job->status == JOB_DEAD) {
        status_str = "dead";
    }

    lua_pushfstring(L, "<job[%s]: %p>", status_str, (void *)h->job);
    return 1;
}