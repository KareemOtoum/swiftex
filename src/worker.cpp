#include "worker.h"
#include "server.h"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <algorithm>

void print_err(std::string_view msg) {
    std::cerr << msg << "\n";
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Worker::handle_new_client() {
    sockaddr_storage client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    epoll_event new_client_ev{};

    int new_client_sock = accept(m_socketfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (new_client_sock == -1) {
        // transient errors are fine
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "accept() error: " << strerror(errno) << "\n";
        }
        return;
    }

    if (set_nonblocking(new_client_sock) == -1) {
        std::cerr << "failed to set client socket non-blocking: " << strerror(errno) << "\n";
        close(new_client_sock);
        return;
    }

    new_client_ev.events = EPOLLIN | EPOLLET; // edge-triggered to avoid busy loops
    new_client_ev.data.fd = new_client_sock;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, new_client_sock, &new_client_ev) == -1) {
        std::cerr << "epoll_ctl add client failed: " << strerror(errno) << "\n";
        close(new_client_sock);
        return;
    }

    m_client_sockets.insert(new_client_sock);
}

void Worker::run(server::MPSCQueueT& request_queue,
                 server::MPSCReturnQueueT& return_response_queue,
                 std::string_view port) {
    server::EpollArray events{};

    std::cout << "worker up\n";
    int socketfd = setup_listening_socket(port);
    if (socketfd == -1) return;
    m_socketfd = socketfd;

    m_epollfd = epoll_create1(0);
    if (m_epollfd == -1) {
        std::cerr << "epoll_create1 failed: " << strerror(errno) << "\n";
        close(m_socketfd);
        return;
    }

    epoll_event ev{};
    ev.data.fd = m_socketfd;
    ev.events = EPOLLIN;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_socketfd, &ev) == -1) {
        std::cerr << "epoll_ctl add listen socket failed: " << strerror(errno) << "\n";
        close(m_socketfd);
        close(m_epollfd);
        return;
    }

    std::cout << "entering worker loop\n";
    run_loop(events, request_queue, return_response_queue);

    shutdown_worker();
}

void Worker::run_loop(server::EpollArray& events,
                      server::MPSCQueueT& request_queue,
                      server::MPSCReturnQueueT& return_response_queue) {

    while (server::g_running) {
        int n = epoll_wait(m_epollfd, events.data(), 
            server::k_maxEvents, server::k_epoll_timeout_ms);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error: " << strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t evts = events[i].events;

            if (fd == m_socketfd) {
                // new client(s). loop accept until EAGAIN for edge-trigger
                while (true) {
                    sockaddr_storage sa{};
                    socklen_t sa_len = sizeof(sa);
                    int new_fd = accept(m_socketfd, (struct sockaddr*)&sa, &sa_len);
                    if (new_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        std::cerr << "accept error in loop: " << strerror(errno) << "\n";
                        break;
                    }
                    if (set_nonblocking(new_fd) == -1) {
                        std::cerr << "failed set_nonblocking for new client\n";
                        close(new_fd);
                        continue;
                    }
                    epoll_event client_ev{};
                    client_ev.data.fd = new_fd;
                    client_ev.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, new_fd, &client_ev) == -1) {
                        std::cerr << "epoll_ctl add client in loop failed: " << strerror(errno) << "\n";
                        close(new_fd);
                        continue;
                    }
                    m_client_sockets.insert(new_fd);
                }
            } else {
                // client packet or hangup
                if (evts & (EPOLLHUP | EPOLLERR)) {
                    // disconnect client
                    epoll_event dummy{};
                    disconnect_client(fd, dummy);
                    continue;
                }

                if (evts & EPOLLIN) {
                    handle_request(fd, events[i], request_queue);
                }
            }
        }

        // check for matcher responses
        handle_responses(request_queue, return_response_queue);
    }
}

void Worker::shutdown_worker() {
    for (int socket : m_client_sockets) {
        close(socket);
    }
    m_client_sockets.clear();

    if (m_socketfd != -1) {
        close(m_socketfd);
        m_socketfd = -1;
    }
    if (m_epollfd != -1) {
        close(m_epollfd);
        m_epollfd = -1;
    }
}

void Worker::disconnect_client(int clientfd, epoll_event& /*event*/) {
    // Remove from epoll
    if (epoll_ctl(m_epollfd, EPOLL_CTL_DEL, clientfd, nullptr) == -1) {
        // EPOLL_CTL_DEL may fail if fd already closed, ignore
    }

    m_client_sockets.erase(clientfd);
    close(clientfd);
}

static ssize_t read_full(int fd, void* buf, size_t bytes) {
    size_t total = 0;
    char* ptr = static_cast<char*>(buf);
    while (total < bytes) {
        ssize_t r = recv(fd, ptr + total, bytes - total, 0);
        if (r == 0) return 0; // peer closed
        if (r < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // no data now, but because we use blocking loop we should treat as partial
                return -EAGAIN;
            }
            return -errno;
        }
        total += static_cast<size_t>(r);
    }
    return static_cast<ssize_t>(total);
}

static ssize_t write_full(int fd, const void* buf, size_t bytes) {
    size_t total = 0;
    const char* ptr = static_cast<const char*>(buf);
    while (total < bytes) {
        ssize_t w = send(fd, ptr + total, bytes - total, MSG_NOSIGNAL);
        if (w <= 0) {
            if (w == -1 && errno == EINTR) continue;
            if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // would block, caller should retry later
                return -EAGAIN;
            }
            return -errno;
        }
        total += static_cast<size_t>(w);
    }
    return static_cast<ssize_t>(total);
}

void Worker::handle_request(int clientfd,
                            epoll_event& /*event*/,
                            server::MPSCQueueT& request_queue) {
    server::BufferT buffer{};
    // read exactly server::k_buffer_size bytes
    ssize_t got = read_full(clientfd, buffer.data(), buffer.size());
    if (got == 0) { // closed
        epoll_event dummy{};
        disconnect_client(clientfd, dummy);
        return;
    }
    if (got < 0) {
        if (got == -EAGAIN) {
            // partial read - in this simple fixed-size protocol we treat as disconnect or skip
            // Better: implement per-client accumulators if we need partial handling.
            return;
        }
        std::cerr << "recv error on fd " << clientfd << ": " << strerror(-got) << "\n";
        epoll_event dummy{};
        disconnect_client(clientfd, dummy);
        return;
    }

    auto* request = PerThreadMemoryPool<EngineRequest>::acquire();
    // keep requests to release later
    m_prev_requests.push_back(request);

    size_t offset = 0;
    try {
        deserialize_request(buffer, offset, *request);
    } catch (const std::exception& ex) {
        std::cerr << "deserialize_request failed: " << ex.what() << "\n";
        PerThreadMemoryPool<EngineRequest>::release(request);
        m_prev_requests.pop_back();
        return;
    }

    // stamp client id and worker id
    request->client_id = static_cast<uint64_t>(clientfd);
    request->m_worker_id = m_id;

    // push to engine
    request_queue.push(request);
}

void Worker::handle_responses(server::MPSCQueueT& /*request_queue*/,
                              server::MPSCReturnQueueT& return_response_queue) {
    // local buffer for sending
    server::BufferT buffer{};

    EngineResponse* response = nullptr;
    // pop as many as available
    while (m_response_queue && m_response_queue->pop(response)) {
        std::cout << "got response from matching engine\n";

        if (!response) continue;

        // If client not connected any more, drop response but still return it to matcher pool
        if (!m_client_sockets.contains(static_cast<int>(response->client_id))) {
            std::cerr << "client " << response->client_id << " not connected, dropping response\n";
            return_response_queue.push(response);
            continue;
        }

        size_t offset = 0;
        try {
            serialize_response(buffer, offset, *response);
        } catch (const std::exception& ex) {
            std::cerr << "serialize_response failed: " << ex.what() << "\n";
            return_response_queue.push(response);
            continue;
        }

        // send only 'offset' bytes
        ssize_t sent = write_full(static_cast<int>(response->client_id), buffer.data(), offset);
        if (sent < 0) {
            if (sent == -EAGAIN) {
                // Can't send right now; in production you'd queue and retry later.
                std::cerr << "would block sending response to " << response->client_id << "\n";
            } else {
                std::cerr << "send error to " << response->client_id << ": " << strerror(-sent) << "\n";
                epoll_event dummy{};
                disconnect_client(static_cast<int>(response->client_id), dummy);
            }
        }

        // return response object to matcher
        return_response_queue.push(response);

        // free last request on ACK
        if (response->m_event == EventType::ACK) {
            if (!m_prev_requests.empty()) {
                auto prev_req = m_prev_requests.front();
                m_prev_requests.pop_front();
                PerThreadMemoryPool<EngineRequest>::release(prev_req);
            } 
        }
    }
}

int setup_listening_socket(std::string_view port)
{
    struct addrinfo hints{}, *serverinfo = nullptr;
    int rv;

    hints.ai_family = AF_UNSPEC;      // don't care about ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo("127.0.0.1", port.data(), &hints, &serverinfo)) != 0) {
        print_err("Server error: getaddrinfo");
        return -1;
    }

    int socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype,
                          serverinfo->ai_protocol);
    if (socketfd == -1) {
        print_err("Server error: server socket");
        freeaddrinfo(serverinfo);
        return -1;
    }

    if (set_nonblocking(socketfd) == -1) {
        print_err("Server error: failed to set non-blocking");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }

    int enable = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        print_err("Server error: setsockopt SO_REUSEADDR failed");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1) {
        print_err("Server error: setsockopt SO_REUSEPORT failed");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }
#endif

    if (bind(socketfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
        close(socketfd);
        freeaddrinfo(serverinfo);
        print_err("Server error: failed to bind to server socket");
        return -1;
    }

    freeaddrinfo(serverinfo);

    if (listen(socketfd, SOMAXCONN) == -1) {
        print_err("Server error: failed to listen");
        close(socketfd);
        return -1;
    }

    return socketfd;
}
