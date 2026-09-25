#include "cjob.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}

// Inserción ordenada O(N) por wake_at para mantener la cabeza siempre con el job más próximo
void enqueue_job(Job *j) {
    if (!j) return;
    j->next = NULL;

    if (!job_head || j->wake_at < job_head->wake_at) {
        j->next = job_head;
        job_head = j;
        if (!job_tail) job_tail = j;
        return;
    }

    Job *curr = job_head;
    while (curr->next && curr->next->wake_at <= j->wake_at) curr = curr->next;

    j->next = curr->next;
    curr->next = j;
    if (!j->next) job_tail = j;
}

Job* dequeue_job(void) {
    if (!job_head) return NULL;
    Job *j = job_head;
    job_head = job_head->next;
    if (!job_head) job_tail = NULL;
    j->next = NULL;
    return j;
}

static void step_job(lua_State *L, Job *j) {
    if (!j) return;
    int nargs_to_pass = 0;
    double now = get_time_sec();

    if (j->nargs >= 0) {
        nargs_to_pass = j->nargs;
        j->nargs = -1;
    } else {
        double actual_elapsed = now - j->start_time;
        // if (actual_elapsed < 0.0001)
        actual_elapsed = actual_elapsed < 0.0001 ? 0.0001 : actual_elapsed;

        lua_settop(j->co, 0);
        lua_pushnumber(j->co, actual_elapsed);
        nargs_to_pass = 1;
    }

    int status = lua_resume(j->co, nargs_to_pass);
    if (status == LUA_YIELD) {
        int top = lua_gettop(j->co);
        
        if (top >= 2 && lua_isnumber(j->co, 1) && lua_isnumber(j->co, 2)) {
            double delay = lua_tonumber(j->co, 1);
            j->start_time = get_time_sec();
            j->wake_at = j->start_time + delay; 
        } else {
            j->start_time = get_time_sec();
            j->wake_at = j->start_time;
        }

        enqueue_job(j);
    } else {
        if (status != LUA_OK) {
            const char *err = lua_tostring(j->co, -1);
            char err_buf[512];
            snprintf(err_buf, sizeof(err_buf), "[CJob Error]: %s", err ? err : "desconocido");

            j->status = JOB_DEAD;
            if (j->co_ref != LUA_NOREF) {
                luaL_unref(j->co, LUA_REGISTRYINDEX, j->co_ref);
                j->co_ref = LUA_NOREF;
            }

            luaL_error(L, "%s", err_buf);
            return;
        }

        j->status = JOB_DEAD;
        if (j->co_ref != LUA_NOREF) {
            luaL_unref(j->co, LUA_REGISTRYINDEX, j->co_ref);
            j->co_ref = LUA_NOREF;
        }
    }
}

void process_jobs(lua_State *L) {
    // Procesar únicamente los jobs que ya deben despertarse en el instante actual
    while (job_head) {
        double now = get_time_sec();
        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
         * Si el job al frente aún NO vence (está en el futuro), salimos inmediatamente  *
         * para devolver el control al bucle de dibujado de la ventana.                  *
        \* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        if (job_head->status == JOB_RUNNING && job_head->wake_at > 0.0 && now < job_head->wake_at) break;
        if (job_head->status == JOB_RUNNING) {
            // Job *curr = dequeue_job();
            // step_job(L, curr);
            step_job(L, dequeue_job());
        } else {
            // Limpieza de jobs no ejecutables
            dequeue_job();
        }
    }
}

void process_jobs_flush(lua_State *L) {
    while (job_head) {
        double now = get_time_sec();
        if (job_head->status == JOB_RUNNING && job_head->wake_at > 0.0 && now < job_head->wake_at) {
            double diff = job_head->wake_at - now;
            struct timespec req = { (time_t)diff, (long)((diff - (time_t)diff) * 1e9) };
            nanosleep(&req, NULL);
        }
        if (job_head->status == JOB_RUNNING) {
            // Job *curr = dequeue_job();
            // step_job(L, curr);
            step_job(L, dequeue_job());
        } else {
            dequeue_job();
        }
    }
}

int l_cjob_wait(lua_State *L) {
    double seconds = luaL_optnumber(L, 1, 0.0);
    seconds = seconds < 0.0 ? 0.0 : seconds;

    double start_time = get_time_sec();

    lua_settop(L, 0);
    lua_pushnumber(L, seconds);
    lua_pushnumber(L, start_time);

    return lua_yield(L, 2);
}

int l_cjob_async(lua_State *L) {
    while (job_head) {
        double now = get_time_sec();

        if (job_head->status == JOB_RUNNING && job_head->wake_at > now) {
            double diff = job_head->wake_at - now;
            struct timespec req = { (time_t)diff, (long)((diff - (time_t)diff) * 1e9) };
            nanosleep(&req, NULL);
        }

        if (job_head->status == JOB_RUNNING) {
            // Job *curr = dequeue_job();
            // step_job(L, curr);
            step_job(L, dequeue_job());
        } else {
            dequeue_job();
        }
    }
    return 0;
}

int l_sentinel_gc(lua_State *L) {
    process_jobs_flush(L);
    return 0;
}