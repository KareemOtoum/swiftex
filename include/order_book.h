#pragma once

#include "order.h"
#include <list>
#include <map>
#include <iostream>

struct OrderBook
{
    using OrderList = std::list<Order>;
    using Price = double;

    std::map<Price, OrderList, std::greater<Price>> bids;
    std::map<Price, OrderList, std::less<Price>> asks;
};


inline std::ostream &operator<<(std::ostream &out, const OrderBook &ob)
{
    out << "BIDS\n";
    for (auto &[price, orderlist] : ob.bids)
    {
        for (auto &order : orderlist)
        {
            out << static_cast<uint32_t>(order.m_remaining_quantity) << " @ " << "$" << price << "\n";
        }
    }
    out << "ASKS\n";
    for (auto &[price, orderlist] : ob.asks)
    {
        for (auto &order : orderlist)
        {
            out << static_cast<uint32_t>(order.m_remaining_quantity) << " @ " << "$" << price << "\n";
        }
    }
    return out;
};
