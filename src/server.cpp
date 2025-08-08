#include "server.h"

namespace server {
    std::atomic<bool> g_running{ true };
}

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

    int workers_size = workers.size();

    for(int i{}; i < workers_size; ++i) {
        workers[i].m_id = i;
    }

    MapBasedMatcher matching_engine{};

    // add worker spsc response queues to matching engine
    auto& response_queue_list = matching_engine.get_response_queue_list_impl();
    response_queue_list.resize(workers.size(), nullptr);
    
    for(int i{}; i < workers_size; ++i) {
        auto queue = std::make_shared<server::SPSCQueueT>();
        workers[i].m_response_queue = queue;
        response_queue_list[i] = queue;
    }

    // start workers
    for(auto& worker : workers) {
        worker.m_thread = std::thread(
            &Worker::run, 
            &worker, 
            std::ref(matching_engine.get_request_queue_impl()), 
            std::ref(matching_engine.get_response_return_queue_impl()), 
            port
        );
    }

    matching_engine.run_impl();

    std::cout << "shutting down server\n";
    for(auto& worker : workers) {
        worker.m_thread.join();
    }
}
