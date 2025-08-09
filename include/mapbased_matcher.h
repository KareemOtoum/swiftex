#pragma once

#include "matching_engine.h"
#include "worker.h"
#include "memory.h"
#include <algorithm>
#include "mpsc_queue.h"

class MapBasedMatcher : public MatchingEngine<MapBasedMatcher> {
    
public:

    void run_impl();

    void process_request_impl(EngineRequest&);

    auto& get_orderbook() { return m_orderbook; }
    auto& get_request_queue_impl() { return m_request_queue; }
    auto& get_response_queue_list_impl() { return m_response_queue_list; }
    auto& get_response_return_queue_impl() { return m_response_return_queue; };

private:
    void handle_add(EngineRequest&);

    void handle_limit_buy(EngineRequest&);
    void handle_limit_sell(EngineRequest&);

    void handle_market_buy(EngineRequest&);
    void handle_market_sell(EngineRequest&);

    void send_worker_response(EngineResponse*);

    MapBasedOrderBook m_orderbook{};
    server::MPSCQueueT m_request_queue;
    std::vector<
        std::shared_ptr<server::SPSCQueueT>> 
        m_response_queue_list;
    server::MPSCReturnQueueT m_response_return_queue;
};
