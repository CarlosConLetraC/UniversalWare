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

void process_jobs(lua_State *L) {
    (void)L;
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
                    // Desencolar
                    if (prev) prev->next = next;
                    else job_head = next;
                    if (curr == job_tail) job_tail = prev;

                    // Reanudar corrutina en su propia pila limpia
                    int status = lua_resume(curr->co, 0);

                    if (status == LUA_YIELD) {
                        double delay = 0.0;
                        int top = lua_gettop(curr->co);
                        if (top > 0 && lua_isnumber(curr->co, top)) {
                            delay = lua_tonumber(curr->co, top);
                        }
                        curr->wake_at = get_time_sec() + delay;
                        enqueue_job(curr);
                    } else {
                        if (status != LUA_OK) {
                            const char *err = lua_tostring(curr->co, -1);
                            fprintf(stderr, "[CJob Error]: %s\n", err ? err : "desconocido");
                        }
                        curr->status = JOB_DEAD;
                        if (curr->co_ref != LUA_NOREF) {
                            luaL_unref(curr->co, LUA_REGISTRYINDEX, curr->co_ref);
                            curr->co_ref = LUA_NOREF;
                        }
                    }
                    active_jobs++;
                    break; // Re-evaluar la lista tras el yield
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
        
        // Ceder un milisegundo a la CPU si hay tareas dormidas esperando tiempo
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
    (void)L;

    while (job_head != NULL) {
        Job *curr = dequeue_job(); // Tomar el primer job
        if (!curr) break;

        if (curr->status == JOB_RUNNING) {
            // Ejecuta EXACTAMENTE 1 OpCode antes de caer en el lua_yield del hook
            int status = lua_resume(curr->co, 0);

            if (status == LUA_YIELD) {
                // El subproceso cedió el turno por el OpCode hook.
                // Lo volvemos a encolar al final para la siguiente instrucción.
                enqueue_job(curr);
            } else {
                // El subproceso finalizó (LUA_OK) o dio error
                if (status != LUA_OK) {
                    const char *err = lua_tostring(curr->co, -1);
                    fprintf(stderr, "[CJob OpCode Error]: %s\n", err ? err : "desconocido");
                }
                curr->status = JOB_DEAD;
                if (curr->co_ref != LUA_NOREF) {
                    luaL_unref(curr->co, LUA_REGISTRYINDEX, curr->co_ref);
                    curr->co_ref = LUA_NOREF;
                }
                free(curr);
            }
        }
    }
    return 0;
}
// Sentinel que corre AL FINAL del script principal, pero fuera de las instrucciones de la VM
int l_sentinel_gc(lua_State *L) {
    process_jobs(L);
    return 0;
}