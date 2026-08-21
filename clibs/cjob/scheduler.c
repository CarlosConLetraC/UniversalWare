#include "cjob.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

// Hook seguro (no hace yield interno de C, evita el crash de LuaJIT)
void cjob_instruction_hook(lua_State *L, lua_Debug *ar) {
    (void)L;
    (void)ar;
}

void *cjob_worker_loop(void *arg) {
    lua_State *main_L = (lua_State *)arg;
    printf("[DEBUG Worker] Hilo de trabajo iniciado correctamente.\n");

    while (worker_running) {
        pthread_mutex_lock(&cjob_mutex);
        
        while (job_head == NULL && worker_running) {
            pthread_cond_wait(&cjob_cond, &cjob_mutex);
        }

        if (!worker_running) {
            pthread_mutex_unlock(&cjob_mutex);
            break;
        }

        Job *j = dequeue_job();
        pthread_mutex_unlock(&cjob_mutex);

        if (!j) continue;

        pthread_mutex_lock(&cjob_mutex);
        int current_status = j->status;
        double wake_at = j->wake_at;
        pthread_mutex_unlock(&cjob_mutex);

        if (current_status != JOB_RUNNING) {
            if (current_status == JOB_DEAD) {
                pthread_mutex_lock(&cjob_mutex);
                if (j->co_ref != LUA_NOREF && main_L != NULL) {
                    luaL_unref(main_L, LUA_REGISTRYINDEX, j->co_ref);
                    j->co_ref = LUA_NOREF;
                }
                free(j);
                pthread_mutex_unlock(&cjob_mutex);
            }
            continue;
        }

        double now = get_time_sec();
        if (now < wake_at) {
            // Aún debe esperar: re-encolar y pausar microsegundos
            pthread_mutex_lock(&cjob_mutex);
            enqueue_job(j);
            pthread_mutex_unlock(&cjob_mutex);
            usleep(500); 
            continue;
        }

        // Ejecución segura de la corrutina
        // En LuaJIT/Lua 5.1, lua_resume recibe la corrutina y el número de args (0 al reanudar)
        int status = lua_resume(j->co, 0);

        if (status == LUA_YIELD) {
            double delay = 0.0;
            if (lua_gettop(j->co) > 0 && lua_isnumber(j->co, -1)) {
                delay = lua_tonumber(j->co, -1);
            }

            pthread_mutex_lock(&cjob_mutex);
            j->wake_at = get_time_sec() + delay;
            enqueue_job(j);
            pthread_mutex_unlock(&cjob_mutex);

        } else if (status == LUA_OK) {
            pthread_mutex_lock(&cjob_mutex);
            j->status = JOB_DEAD;
            if (j->co_ref != LUA_NOREF && main_L != NULL) {
                luaL_unref(main_L, LUA_REGISTRYINDEX, j->co_ref);
                j->co_ref = LUA_NOREF;
            }
            free(j);
            pthread_mutex_unlock(&cjob_mutex);

        } else {
            // Manejo de errores de tiempo de ejecución
            const char *err = lua_tostring(j->co, -1);
            fprintf(stderr, "[ERROR Worker] Error en Job (%p): %s\n", (void*)j, err ? err : "error desconocido");

            pthread_mutex_lock(&cjob_mutex);
            j->status = JOB_DEAD;
            if (j->co_ref != LUA_NOREF && main_L != NULL) {
                luaL_unref(main_L, LUA_REGISTRYINDEX, j->co_ref);
                j->co_ref = LUA_NOREF;
            }
            free(j);
            pthread_mutex_unlock(&cjob_mutex);
        }
    }
    return NULL;
}