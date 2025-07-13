#pragma once

#include "order.h"
#include "order_book.h"
#include <functional>
#include <variant>
#include <cassert>
#include <iostream>
#include <vector> 

struct Trade {
    uint64_t m_resting_order_id{};
    uint64_t m_incoming_order_id{};
    double m_price{};
    uint32_t m_quantity{};
};

enum class EngineCommand {
    ADD_ORDER,
    MODIFY_ORDER,
    CANCEL_ORDER
};

struct EngineRequest {
    Order m_order{};
    uint64_t client_id{};
    EngineCommand m_cmd{};
};

enum class EventType {
    ACK,
    TRADE_EXECUTED,
    REJECT,
    CANCEL_ACK
};

struct EngineResponse {
    std::variant<Trade, Order, uint64_t> m_payload{};
    EventType m_event{};
    uint64_t client_id{};
};


class MatchingEngine {

public:
    using CallbackFunc = std::function<void(EngineResponse)>;

    void process_request(EngineRequest& req, 
        CallbackFunc event_handler);
    
        auto& get_orderbook() { return m_orderbook; }

private:
    void handle_add(EngineRequest& req, 
        CallbackFunc event_handler);

    OrderBook m_orderbook;
};