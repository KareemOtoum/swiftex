#include "worker.h"
#include "server.h"

void print_err(std::string_view msg) {
    std::cerr << msg << "\n";
}

void Worker::run(MPSCQueue<EngineRequest>& request_queue,
    std::string_view port) {
    
    server::EpollArray events{};

    std::cout << "worker up\n"; 
    // setup listening socket
    int socketfd = setup_listening_socket(port);
    if(socketfd == -1) return;
    m_socketfd = socketfd;

    m_epollfd = epoll_create1(0);

    epoll_event ev{};
    ev.data.fd = m_socketfd;
    ev.events = EPOLLIN;
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_socketfd, &ev);

    std::cout << "entering worker loop\n";
    run_loop(events, request_queue);
}

void Worker::run_loop(server::EpollArray& events,
    MPSCQueue<EngineRequest>& request_queue) {
    while(server::g_running) {
        int n = epoll_wait(m_epollfd, events.data(), server::k_maxEvents, 
            server::k_epoll_timeout_ms);

        for(int i{}; i < n; ++i) {
            int fd = events[i].data.fd;
        }
    }
}

int setup_listening_socket(std::string_view port)
{
    struct addrinfo hints, *serverinfo;
    int rv;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // don't care about ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    
    if((rv = getaddrinfo("127.0.0.1", port.data(), &hints, &serverinfo)) != 0) {
        print_err("Server error: getaddrinfo");
        return -1;
    }
    
    int socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype,
                         serverinfo->ai_protocol);
    if(socketfd == -1) {
        print_err("Server error: server socket");
        freeaddrinfo(serverinfo);
        return -1;
    }
    
    // Set socket to non-blocking
    int flags = fcntl(socketfd, F_GETFL, 0);
    if (flags == -1 || fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        print_err("Server error: failed to set non-blocking");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }
    
    int enable = 1;
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        print_err("Server error: setsockopt SO_REUSEADDR failed");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }
    
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1) {
        print_err("Server error: setsockopt SO_REUSEPORT failed");
        close(socketfd);
        freeaddrinfo(serverinfo);
        return -1;
    }
    
    if(bind(socketfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
        close(socketfd);
        freeaddrinfo(serverinfo);
        print_err("Server error: failed to bind to server socket");
        return -1;
    }
    
    freeaddrinfo(serverinfo);
    
    if(listen(socketfd, SOMAXCONN) == -1) {
        print_err("Server error: failed to listen");
        close(socketfd);
        return -1;
    }
    
    return socketfd;
}