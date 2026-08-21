#ifndef CJOB_H
    #define CJOB_H

    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
    #include <pthread.h>
    #include <stdbool.h>

    #define CJOB_MT "cjob.handle"

    typedef enum {
        JOB_RUNNING,
        JOB_SUSPENDED,
        JOB_DEAD
    } JobStatus;

    typedef struct Job {
        lua_State *co;
        int co_ref;
        JobStatus status;
        double wake_at;
        bool is_in_queue;
        struct Job *next;
    } Job;

    typedef struct {
        Job *job;
    } JobHandle;

    // Colas de Trabajos y Sincronización
    extern Job *job_head;
    extern Job *job_tail;
    extern pthread_t worker_thread;
    extern pthread_mutex_t cjob_mutex;
    extern pthread_cond_t cjob_cond;
    extern bool worker_running;

    extern Job *job_head;
    extern Job *job_tail;

    // Funciones del Scheduler (scheduler.c)
    void* cjob_worker_loop(void *arg);
    void cjob_instruction_hook(lua_State *L, lua_Debug *ar);
    
    // Métodos de JobHandle expuestos desde job.c para main.c
    int l_job_index(lua_State *L);
    int l_job_gc(lua_State *L);
    int l_job_kill(lua_State *L);
    int l_job_stop(lua_State *L);
    int l_job_resume(lua_State *L);
    
    // API principal para Lua (main.c)
    int l_cjob_new(lua_State *L);
    int l_cjob_wait(lua_State *L);
    void enqueue_job(Job *j);
    Job* dequeue_job(void);
#endif // CJOB_H