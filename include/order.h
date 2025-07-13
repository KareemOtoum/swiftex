#pragma once

#include <cstdint>
#include <string>

enum class Side { 
    BUY,
    SELL
};

enum class Type {
    MARKET,
    LIMIT
};

struct Order {
    std::string m_symbol{};
    uint64_t m_order_id{};
    double m_price{};
    uint32_t m_quantity{};
    uint32_t m_remaining_quantity{};

    Side m_side{};
    Type m_type{};
};