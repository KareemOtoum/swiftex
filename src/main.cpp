#include "mapbased_matcher.h"
#include "perthread_memory_pool.h"
#include <iostream>

void test1();
void test2();

int main() {
    
    test2();
    return 0;
}

template<typename T>
void take(MatchingEngine<T>& engine) {

}

void event_handler(const EngineResponse& res);

MapBasedMatcher engine;

void test1() {
    
}

void test2() {
    engine.get_orderbook().bids[50].push_back(
        Order{
            .m_price { 50 },
            .m_remaining_quantity { 100 }
        }
    );
    engine.get_orderbook().bids[50].push_back(
        Order{
            .m_price { 40 },
            .m_remaining_quantity { 50 }
        }
    );

    auto req_ptr = PerThreadMemoryPool<EngineRequest>::acquire();
    *req_ptr = {
        .m_cmd = EngineCommand::ADD_ORDER
    };
    auto order_ptr = PerThreadMemoryPool<Order>::acquire();
    order_ptr->m_price = 50;
    order_ptr->m_quantity = 150;
    order_ptr->m_remaining_quantity = 150;
    order_ptr->m_side = order::Side::SELL;
    order_ptr->m_type = order::Type::LIMIT;
    req_ptr->m_order = *order_ptr;

    std::cout << engine.get_orderbook() << "\n";

    engine.process_request(*req_ptr, event_handler);
}

void event_handler(const EngineResponse& res)
{
    if(res.m_event == EventType::TRADE_EXECUTED) {
        std::cout << "Trade executed ";
        auto& trade = std::get<Trade>(res.m_payload);
        std::cout << "price: " << trade.m_price <<
            " quantity: " << trade.m_quantity << "\n";
    } else {
        auto& order = std::get<Order>(res.m_payload);
        std::cout << "ACK remaining quantity: " << 
            order.m_remaining_quantity << " price: " << 
            order.m_price << "\n" <<
            "id: " << order.m_id << "\n";
    }
    std::cout << engine.get_orderbook() << "\n";
}
