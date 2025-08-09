#pragma once

#include "order.h"
#include "mapbased_orderbook.h"
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
    uint8_t m_worker_id{};
};

enum class EventType {
    ACK,
    TRADE_EXECUTED,
    REJECT,
    CANCEL_ACK
};

struct EngineResponse {
    std::variant<Trade, Order, uint64_t> m_payload{};
    uint64_t client_id{};
    EventType m_event{};
    uint8_t m_worker_id{};
};

template<typename Derived>
class MatchingEngine {
public:
    void run() {
        static_cast<Derived*>(this)->run_impl();
    }

    auto& get_request_queue() {
        return static_cast<Derived*>(this)->get_request_queue_impl();
    }

    auto& get_response_return_queue() {
        return static_cast<Derived*>(this)->get_response_return_queue_impl();
    }

    auto& get_response_queue_list() {
        return static_cast<Derived*>(this)->get_response_queue_list_impl();
    }

    void process_request(EngineRequest& req) {
        static_cast<Derived*>(this)->process_request_impl(req);
    }
};