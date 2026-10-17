#include "libbackend/Scheduler.h"
#include "libbackend/Job.h"

#include <filesystem>
#include <signal.h>
#include <iostream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);

    fs::path dir = (argc > 1) ? fs::path(argv[1]) : fs::current_path();

    if (!fs::exists(dir)) {
        std::cerr << "Directorio no existe\n";
        return 1;
    }

    size_t threads = std::max(1u, std::thread::hardware_concurrency() / 2);

    Scheduler scheduler(threads);
    scheduler.start();

    std::cout << "Scheduler iniciado con " << threads << " threads\n";

    int id = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        if (name.rfind("program", 0) != 0) continue;
        if (entry.path().extension() != ".lua") continue;
        bool isServer = (name.find("fetch") != std::string::npos);
        Job job(entry.path().string(), 10, isServer);
        job.persistent = isServer;
        std::cout << "[Main] Job creado ID=" << job.id << " -> " << name << "\n";
        scheduler.submit(job);
    }

    std::cout << "Esperando ejecucion...\n";
    for (;;) std::this_thread::sleep_for(std::chrono::seconds(1));
    scheduler.stopScheduler();

    return 0;
}