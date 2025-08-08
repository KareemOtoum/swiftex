#pragma once

#include "mpsc_queue.h"
#include "spsc_queue.h"
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
    
    // powers of two only
    constexpr int k_response_queue_size{ 4096 };
    constexpr int k_request_queue_size{ 16384 };
    constexpr int k_response_return_queue_size{ 4096 };

    using SPSCQueueT = 
        SPSCQueue<EngineResponse*, server::k_response_queue_size>;

    using MPSCQueueT = 
        MPSCQueue<EngineRequest*, server::k_request_queue_size>;

    using MPSCReturnQueueT = 
        MPSCQueue<EngineResponse*, server::k_response_return_queue_size>;
}

struct Worker {
    Worker() : m_response_queue { std::make_shared<server::SPSCQueueT>() }, 
        m_id { server::generate_worker_id() } { }

    std::shared_ptr<server::SPSCQueueT> m_response_queue;
    std::thread m_thread;

    // epoll instance
    int m_epollfd{};
    int m_socketfd{};

    int m_id;

    void run(server::MPSCQueueT&, server::MPSCReturnQueueT&,
        std::string_view port=server::k_default_port);

    void run_loop(server::EpollArray&, server::MPSCQueueT&, server::MPSCReturnQueueT&);
    void handle_new_client();
};

int setup_listening_socket(std::string_view port);