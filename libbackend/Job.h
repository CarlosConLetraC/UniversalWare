#pragma once

#include <string>
#include <atomic>
#include <chrono>

enum class JobStatus {
    PENDING,
    RUNNING,
    SUCCESS,
    FAILED,
    RETRYING
};

struct JobResult {
    bool success;
    int exitCode;
};

struct Job {
    static inline std::atomic<int> nextId{1};

    int id;
    std::string script;

    int retries = 0;
    int maxRetries = 3;
    int priority = 0;
    bool persistent = false;

    JobStatus status = JobStatus::PENDING;

    std::chrono::steady_clock::time_point runAt;

    /*Job(std::string scriptPath, int prio = 0) : id(nextId++),
        script(std::move(scriptPath)),
        priority(prio),
        runAt(std::chrono::steady_clock::now()) {}*/

    Job(std::string scriptPath, int prio = 0, bool isPersistent = false) 
        : id(nextId++), script(std::move(scriptPath)), priority(prio), persistent(isPersistent),
          runAt(std::chrono::steady_clock::now()) {}
};
