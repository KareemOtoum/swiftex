#include "mapbased_matcher.h"
#include <iostream>

void test1();
void test2();

int main() {
    
    test2();
    return 0;
}

void event_handler(const EngineResponse& res);

MapBasedMatcher engine{};

void test1() {
    engine.get_orderbook().asks[50].push_back(
        Order{
            .m_price { 50 },
            .m_remaining_quantity { 100 }
        }
    );
    engine.get_orderbook().asks[50.50].push_back(
        Order{
            .m_price { 50.50 },
            .m_remaining_quantity { 50 }
        }
    );
    engine.get_orderbook().asks[50.50].push_back(
        Order{
            .m_price { 50.50 },
            .m_remaining_quantity { 220 }
        }
    );
    engine.get_orderbook().asks[20.50].push_back(
        Order{
            .m_price { 20.50 },
            .m_remaining_quantity { 150 }
        }
    );
    engine.get_orderbook().asks[20.50].push_back(
        Order{
            .m_price { 20.50 },
            .m_remaining_quantity { 20 }
        }
    );

    EngineRequest req {
        .m_order = Order{
            .m_price{30.50},
            .m_quantity{200},
            .m_remaining_quantity{200},
            .m_side{order::Side::BUY},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    std::cout << engine.get_orderbook() << "\n";

    engine.process_request(req, event_handler);
}

void test2() {
    engine.get_orderbook().bids[50].push_back(
        Order{
            .m_price { 50 },
            .m_remaining_quantity { 100 }
        }
    );
    engine.get_orderbook().bids[40].push_back(
        Order{
            .m_price { 40 },
            .m_remaining_quantity { 50 }
        }
    );

    EngineRequest req {
        .m_order = Order{
            .m_price{50},
            .m_quantity{150},
            .m_remaining_quantity{ 150 },
            .m_side{order::Side::SELL},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    std::cout << engine.get_orderbook() << "\n";

    engine.process_request(req, event_handler);
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
            order.m_price << "\n";
    }
    std::cout << engine.get_orderbook() << "\n";
}
