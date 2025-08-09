#include "order.h"
#include "matching_engine.h"
#include "server.h"        // for BufferT and serialization funcs

#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

// dummy request creation
EngineRequest make_dummy_request() {
    EngineRequest req{};
    req.m_cmd = EngineCommand::ADD_ORDER;
    req.m_worker_id = 0;
    req.client_id = 0;

    req.m_order.m_symbol = "AAPL";
    req.m_order.m_price = 100.0; // double in your struct
    req.m_order.m_quantity = 10;
    req.m_order.m_remaining_quantity = 10;
    req.m_order.m_side = order::Side::BUY;      // Buy
    req.m_order.m_type = order::Type::LIMIT;
    return req;
}

int connect_to_server(const char* host, const char* port) {
    // ... your existing connect logic, simplified ...
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return sockfd;
}

void client_loop(std::atomic<bool>& running, std::atomic<uint64_t>& sent_count,
                 const char* host, const char* port) {
    int fd = connect_to_server(host, port);
    if (fd == -1) return;

    while (running.load(std::memory_order_relaxed)) {
        server::BufferT buf{};
        size_t offset = 0;
        EngineRequest req = make_dummy_request();
        serialize_request(buf, offset, req);

        ssize_t res = send(fd, buf.data(), offset, 0);
        if (res == (ssize_t)offset) {
            sent_count.fetch_add(1, std::memory_order_relaxed);
        } else if (res == -1) {
            if (errno == EINTR) continue;
            perror("send");
            break;
        }
        // Optionally sleep/yield here if needed
    }
    close(fd);
}

int main(int argc, char** argv) {
    int num_clients = 10;
    int duration = 10;
    const char* host = "127.0.0.1";
    const char* port = "5050";

    if (argc > 1) num_clients = std::stoi(argv[1]);
    if (argc > 2) duration = std::stoi(argv[2]);
    if (argc > 3) host = argv[3];
    if (argc > 4) port = argv[4];

    std::atomic<bool> running(true);
    std::atomic<uint64_t> total_sent(0);

    std::vector<std::thread> threads;
    for (int i = 0; i < num_clients; ++i) {
        threads.emplace_back(client_loop, std::ref(running), std::ref(total_sent), host, port);
    }

    std::this_thread::sleep_for(std::chrono::seconds(duration));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Total requests sent: " << total_sent.load() << "\n";
    std::cout << "Requests per second: " << total_sent.load() / (double)duration << "\n";

    return 0;
}
