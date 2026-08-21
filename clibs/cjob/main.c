#include "cjob.h"
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

pthread_mutex_t cjob_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cjob_cond  = PTHREAD_COND_INITIALIZER;

Job *job_head = NULL;
Job *job_tail = NULL;

pthread_t worker_thread;
bool worker_running = false;

void enqueue_job(Job *j) {
    if (!j) return;
    j->next = NULL;
    j->is_in_queue = true;

    if (job_tail == NULL) {
        job_head = j;
        job_tail = j;
    } else {
        job_tail->next = j;
        job_tail = j;
    }
}

Job* dequeue_job(void) {
    if (job_head == NULL) return NULL;

    Job *j = job_head;
    job_head = job_head->next;
    if (job_head == NULL) {
        job_tail = NULL;
    }

    j->next = NULL;
    j->is_in_queue = false;
    return j;
}

int l_cjob_new(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    // 1. Crear el nuevo hilo/corrutina de Lua
    lua_State *co = lua_newthread(L);

    // 2. Copiar la función del argumento 1 al stack de la nueva corrutina 'co' FIRST
    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);

    // 3. Anclar la corrutina 'co' en el Registry para evitar que el GC la elimine
    // (Ahora 'co' está en el top de L, por lo que luaL_ref lo consume limpiamente)
    int co_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // 4. Asignar e inicializar la estructura Job
    Job *j = (Job *)malloc(sizeof(Job));
    if (!j) {
        return luaL_error(L, "Out of memory allocating Job structure");
    }
    
    j->co = co;
    j->co_ref = co_ref;
    j->status = JOB_RUNNING;
    j->wake_at = 0.0;
    j->is_in_queue = true;
    j->next = NULL;

    // Configuración opcional del Preemptive Hook
    lua_sethook(co, cjob_instruction_hook, LUA_MASKCOUNT, 1000);

    // 5. Encolar el trabajo de forma atómica y notificar al worker
    pthread_mutex_lock(&cjob_mutex);
    enqueue_job(j);
    pthread_cond_signal(&cjob_cond);
    pthread_mutex_unlock(&cjob_mutex);

    // 6. Crear y retornar el Userdata Handle a Lua
    JobHandle *handle = (JobHandle *)lua_newuserdata(L, sizeof(JobHandle));
    handle->job = j;
    luaL_getmetatable(L, CJOB_MT);
    lua_setmetatable(L, -2);

    return 1;
}

// Obtener el tiempo actual en segundos con precisión de nanosegundos
static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

// Función cjob.wait(seconds)
int l_cjob_wait(lua_State *L) {
    double delay = luaL_optnumber(L, 1, 0.0);
    double wake_at = get_time_sec() + delay;
    lua_pushnumber(L, wake_at);
    return lua_yield(L, 1);
}

static int l_cjob_cleanup(lua_State *L) {
    if (worker_running) {
        pthread_mutex_lock(&cjob_mutex);
        worker_running = false;
        pthread_cond_signal(&cjob_cond);
        pthread_mutex_unlock(&cjob_mutex);

        pthread_join(worker_thread, NULL);
        pthread_mutex_destroy(&cjob_mutex);
        pthread_cond_destroy(&cjob_cond);
    }
    return 0;
}

static int cjob_auto_join_gc(lua_State *L) {
    (void)L;
    // Esperar a que la cola de trabajos quede totalmente vacía
    while (1) {
        pthread_mutex_lock(&cjob_mutex);
        bool is_empty = (job_head == NULL);
        pthread_mutex_unlock(&cjob_mutex);
        if (is_empty) break;

        // Ceder tiempo de CPU al hilo worker
        usleep(5000); // 5 milisegundos
    }
    return 0;
}

static const struct luaL_Reg cjob_funcs[] = {
    {"new",  l_cjob_new},
    // {"step", l_cjob_step},
    {"wait", l_cjob_wait},
    {NULL, NULL}
};

int luaopen_cjob(lua_State *L) {
    // 1. Configurar metatabla para los Handles de CJob
    luaL_newmetatable(L, CJOB_MT);
    lua_pushcfunction(L, l_job_index);
    lua_setfield(L, -2, "__index");
    
    lua_pushcfunction(L, l_job_gc);
    lua_setfield(L, -2, "__gc");
    
    lua_pushcfunction(L, l_job_kill);
    lua_setfield(L, -2, "kill");
    
    lua_pushcfunction(L, l_job_stop);
    lua_setfield(L, -2, "stop");
    
    lua_pushcfunction(L, l_job_resume);
    lua_setfield(L, -2, "resume");
    lua_pop(L, 1);

    // 2. Registrar las funciones del módulo cjob
    luaL_newlib(L, cjob_funcs);

    // 3. Arrancar el Hilo Worker si no está activo
    if (!worker_running) {
        worker_running = true;
        pthread_create(&worker_thread, NULL, cjob_worker_loop, L);
    }

    // 4. Registrar metatabla de limpieza global del hilo (worker_running = false)
    lua_newuserdata(L, 1);
    luaL_newmetatable(L, "cjob_cleanup_mt");
    lua_pushcfunction(L, l_cjob_cleanup);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    luaL_ref(L, LUA_REGISTRYINDEX);

    // 5. Registrar Proxy de Espera Automática (__gc en LUA_REGISTRYINDEX)
    luaL_newmetatable(L, "cjob_clean_proxy_mt");
    lua_pushcfunction(L, cjob_auto_join_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);

    lua_newuserdata(L, 1);
    luaL_getmetatable(L, "cjob_clean_proxy_mt");
    lua_setmetatable(L, -2);
    luaL_ref(L, LUA_REGISTRYINDEX); // Anclado hasta el shutdown final del interprete

    return 1;
}