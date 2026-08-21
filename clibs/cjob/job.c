#include "cjob.h"
#include <string.h>
#include <stdlib.h>

int l_job_kill(lua_State *L) {
    JobHandle *handle = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (!handle || !handle->job) return 0;

    pthread_mutex_lock(&cjob_mutex);
    Job *j = handle->job;
    if (j && j->status != JOB_DEAD) {
        j->status = JOB_DEAD;
    }
    pthread_mutex_unlock(&cjob_mutex);
    return 0;
}

int l_job_stop(lua_State *L) {
    JobHandle *handle = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (!handle || !handle->job) return 0;

    pthread_mutex_lock(&cjob_mutex);
    Job *j = handle->job;
    if (j && j->status == JOB_RUNNING) {
        j->status = JOB_SUSPENDED;
    }
    pthread_mutex_unlock(&cjob_mutex);
    return 0;
}

int l_job_resume(lua_State *L) {
    JobHandle *handle = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (!handle || !handle->job) return 0;

    pthread_mutex_lock(&cjob_mutex);
    Job *j = handle->job;
    if (j && j->status == JOB_SUSPENDED) {
        j->status = JOB_RUNNING;
        j->wake_at = 0.0;
        if (!j->is_in_queue) {
            enqueue_job(j);
            pthread_cond_signal(&cjob_cond);
        }
    }
    pthread_mutex_unlock(&cjob_mutex);
    return 0;
}

int l_job_index(lua_State *L) {
    JobHandle *handle = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    const char *key = luaL_checkstring(L, 2);

    if (strcmp(key, "status") == 0) {
        pthread_mutex_lock(&cjob_mutex);
        Job *j = handle->job;
        const char *status_str = "dead";

        if (j && j->status != JOB_DEAD) {
            if (j->status == JOB_SUSPENDED) {
                status_str = "suspended";
            } else {
                status_str = "running";
            }
        }
        pthread_mutex_unlock(&cjob_mutex);

        lua_pushstring(L, status_str);
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    return 1;
}

int l_job_gc(lua_State *L) {
    (void)L;
    JobHandle *handle = (JobHandle *)luaL_checkudata(L, 1, CJOB_MT);
    if (handle) {
        handle->job = NULL;
    }
    return 0;
}