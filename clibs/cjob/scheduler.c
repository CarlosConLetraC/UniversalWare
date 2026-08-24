#include "cjob.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int in_scheduler = 0;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

void enqueue_job(Job *j) {
    if (!j) return;
    j->next = NULL;
    if (!job_tail) {
        job_head = job_tail = j;
    } else {
        job_tail->next = j;
        job_tail = j;
    }
}

Job* dequeue_job(void) {
    if (!job_head) return NULL;
    Job *j = job_head;
    job_head = job_head->next;
    if (!job_head) job_tail = NULL;
    j->next = NULL;
    return j;
}

// Recibe lua_State *L para poder lanzar luaL_error hacia el entorno principal
static void step_job(lua_State *L, Job *j) {
    int nargs_to_pass = 0;
    if (j->nargs >= 0) {
        nargs_to_pass = j->nargs;
        j->nargs = -1; // Marcamos que los argumentos ya fueron consumidos
    }

    int status = lua_resume(j->co, nargs_to_pass);

    if (status == LUA_YIELD) {
        double delay = 0.0;
        int top = lua_gettop(j->co);
        
        if (top > 0 && lua_isnumber(j->co, top))
            delay = lua_tonumber(j->co, top);

        j->wake_at = get_time_sec() + delay;
        enqueue_job(j);
    } else {
        // En caso de error en la corrutina:
        if (status != LUA_OK) {
            // Extraer mensaje de error del stack de la corrutina
            const char *err = lua_tostring(j->co, -1);
            char err_buf[512];
            snprintf(err_buf, sizeof(err_buf), "[CJob Error]: %s", err ? err : "desconocido");

            // Limpieza del Job antes de interrumpir la ejecución
            j->status = JOB_DEAD;
            if (j->co_ref != LUA_NOREF) {
                luaL_unref(j->co, LUA_REGISTRYINDEX, j->co_ref);
                j->co_ref = LUA_NOREF;
            }

            // Elevar el error al lua_State principal
            luaL_error(L, "%s", err_buf);
            return;
        }

        // Finalización exitosa
        j->status = JOB_DEAD;
        if (j->co_ref != LUA_NOREF) {
            luaL_unref(j->co, LUA_REGISTRYINDEX, j->co_ref);
            j->co_ref = LUA_NOREF;
        }
    }
}

void process_jobs(lua_State *L) {
    if (!job_head || in_scheduler) return;

    in_scheduler = 1;

    while (job_head) {
        Job *prev = NULL;
        Job *curr = job_head;
        double now = get_time_sec();
        int active_jobs = 0;

        while (curr) {
            Job *next = curr->next;

            if (curr->status == JOB_RUNNING) {
                if (now >= curr->wake_at) {
                    if (prev) prev->next = next;
                    else job_head = next;
                    if (curr == job_tail) job_tail = prev;

                    // Pasamos 'L' para que cualquier error detenga el scheduler y salte a Lua
                    step_job(L, curr);

                    active_jobs++;
                    break;
                } else {
                    active_jobs++;
                    prev = curr;
                }
            } else {
                prev = curr;
            }
            curr = next;
        }

        if (active_jobs == 0) break;
        usleep(1000);
    }

    in_scheduler = 0;
}

int l_cjob_wait(lua_State *L) {
    double seconds = luaL_optnumber(L, 1, 0.0);
    process_jobs(L);
    lua_settop(L, 0);
    lua_pushnumber(L, seconds);
    return lua_yield(L, 1);
}

int l_cjob_async(lua_State *L) {
    while (job_head != NULL) {
        Job *curr = dequeue_job();
        if (!curr) break;
        if (curr->status == JOB_RUNNING)step_job(L, curr);
    }
    return 0;
}

int l_sentinel_gc(lua_State *L) {
    process_jobs(L);
    return 0;
}