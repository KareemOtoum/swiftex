#pragma once

#include "matching_engine.h"
#include "worker.h"

#include <thread>
#include <cstring>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <csignal>
#include <stdio.h>
#include <string>

#include <sys/socket.h>
#include <sys/types.h>

#include <fcntl.h>
#include <sys/epoll.h>

#include <sys/unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <chrono>
#include <errno.h>

namespace server
{
    extern std::atomic<bool> g_running;

    inline const int k_worker_count { 
        static_cast<int>(std::thread::hardware_concurrency())};
    constexpr int k_default_worker_count { 4 };
}

struct Worker;

template <typename Matcher>
void start_server(std::string_view port) {
    using namespace server;

    auto handle_exit = [](int signal) {
        g_running = false;
    };

    std::signal(SIGINT, handle_exit);
    std::signal(SIGTERM, handle_exit);

    std::vector<Worker> workers(
        k_worker_count > 0 ? k_worker_count : k_default_worker_count
    );

    MatchingEngine<Matcher> matching_engine;

    // add worker spsc response queues to matching engine
    matching_engine.get_response_queue_list.resize(workers.size());
    for(auto& worker : workers) {
        assert(worker.m_id < workers.size() && worker.m_id >= 0);
        matching_engine.get_response_queue_list[worker.m_id] = worker.m_response_queue;
    }

    // start workers
    for(auto& worker : workers) {
        worker.m_thread = std::thread(&Worker::run, &worker, 
            std::ref(matching_engine.get_request_queue()), port);
    }

    matching_engine.run();

    std::cout << "shutting down server\n";
    for(auto& worker : workers) {
        worker.m_thread.join();
    }
}
