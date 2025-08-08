#pragma once

#include "matching_engine.h"
#include "mapbased_matcher.h"
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

void start_server(std::string_view port);