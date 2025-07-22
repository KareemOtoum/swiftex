#pragma once

#include "perthread_memory_pool.h"
#include <cstdint>
#include <string>
#include <atomic>

namespace order {

    uint64_t generate_order_id();

    enum class Side { 
        BUY,
        SELL
    };

    enum class Type {
        MARKET,
        LIMIT
    };
}

struct Order {
    std::string m_symbol{};
    double m_price{};
    uint64_t m_id{};
    uint32_t m_quantity{};
    uint32_t m_remaining_quantity{};

    order::Side m_side{};
    order::Type m_type{};
};

// called when objects are acquired from the pool
template<>
struct PoolInitializer<Order> {
    static void init(Order* obj) {
        obj->m_id = order::generate_order_id();
    }
};