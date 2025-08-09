#pragma once

#include "mpsc_queue.h"
#include "spsc_queue.h"
#include "matching_engine.h"
#include "swift_protocol.h"

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
    constexpr int k_epoll_timeout_ms{ 10 };
    
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

    uint64_t generate_worker_id();
};

struct Worker {
    std::shared_ptr<server::SPSCQueueT> m_response_queue;
    std::thread m_thread;
    std::unordered_set<int> m_client_sockets;
    std::deque<EngineRequest*> m_prev_requests;
    std::unordered_map<int, std::vector<uint8_t>> m_client_buffers;

    // epoll instance
    int m_epollfd{};
    int m_socketfd{};

    uint64_t m_id;

    void run(server::MPSCQueueT&, server::MPSCReturnQueueT&,
        std::string_view port=server::k_default_port);

    void run_loop(server::EpollArray&, server::MPSCQueueT&, server::MPSCReturnQueueT&);
    void handle_new_client();
    void handle_request(int clientfd, epoll_event&, server::MPSCQueueT&);
    void handle_responses(server::MPSCQueueT&, server::MPSCReturnQueueT&);
    void disconnect_client(int clientfd, epoll_event&);
    void shutdown_worker();
};

int setup_listening_socket(std::string_view port);