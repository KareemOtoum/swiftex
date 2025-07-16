#pragma once

#include "matching_engine.h"

class MapBasedMatcher : public MatchingEngine<MapBasedMatcher> {
    
public:
    void process_request_impl(EngineRequest&, CallbackFunc);

    auto& get_orderbook() { return m_orderbook; }

private:
    void handle_add(EngineRequest&, CallbackFunc);

    void handle_limit_buy(EngineRequest&, CallbackFunc);
    void handle_limit_sell(EngineRequest&, CallbackFunc);

    void handle_market_buy(EngineRequest&, CallbackFunc);
    void handle_market_sell(EngineRequest&, CallbackFunc);

    MapBasedOrderBook m_orderbook{};
};