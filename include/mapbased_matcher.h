#pragma once

#include "matching_engine.h"
#include <algorithm>
#include "mpsc_queue.h"
#include "server.h"

class MapBasedMatcher : public MatchingEngine<MapBasedMatcher> {
    
public:
    void run_impl();

    void process_request_impl(EngineRequest&, CallbackFunc);

    auto& get_orderbook() { return m_orderbook; }
    auto& get_request_queue_impl() { return m_request_queue; }

private:
    void handle_add(EngineRequest&, CallbackFunc);

    void handle_limit_buy(EngineRequest&, CallbackFunc);
    void handle_limit_sell(EngineRequest&, CallbackFunc);

    void handle_market_buy(EngineRequest&, CallbackFunc);
    void handle_market_sell(EngineRequest&, CallbackFunc);

    MapBasedOrderBook m_orderbook{};
    MPSCQueue<EngineRequest> m_request_queue;
};