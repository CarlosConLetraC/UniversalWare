#ifndef CJOB_H
    #define CJOB_H

    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
    #include <stdbool.h>

    #define CJOB_MT "CJob.Handle"
    #define CJOB_SENTINEL_MT "CJob.Sentinel"

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
        struct Job *next;
    } Job;

    typedef struct {
        Job *job;
    } JobHandle;

    extern Job *job_head;
    extern Job *job_tail;

    void enqueue_job(Job *j);
    Job* dequeue_job(void);
    void process_jobs(lua_State *L);
    // void cjob_vm_hook(lua_State *L, lua_Debug *ar);

    // Funciones expuestas a Lua
    int l_cjob_new(lua_State *L);
    int l_cjob_wait(lua_State *L);
    int l_cjob_async(lua_State *L);

    // Métodos del Handle (job.c)
    int l_job_kill(lua_State *L);
    int l_job_stop(lua_State *L);
    int l_job_resume(lua_State *L);
    int l_job_index(lua_State *L);
    int l_job_gc(lua_State *L);
    int l_job_tostring(lua_State *L);

    int l_sentinel_gc(lua_State *L);
#endif