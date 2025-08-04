#pragma once

#include "mpsc_queue.h"
#include "matching_engine.h"

#include <thread>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

namespace server {
    constexpr int k_maxEvents{ 1024 };
    constexpr int k_epoll_timeout_ms{ 100 };

    constexpr std::string_view k_default_port { "5050" };
    using EpollArray = std::array<epoll_event, server::k_maxEvents>;
}

struct Worker {
    std::thread m_thread;

    // epoll instance
    int m_epollfd;
    int m_socketfd;

    void run(MPSCQueue<EngineRequest>&,
        std::string_view port=server::k_default_port);

    void run_loop(server::EpollArray&, MPSCQueue<EngineRequest>&);
};

int setup_listening_socket(std::string_view port);