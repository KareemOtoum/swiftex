#include "matching_engine.h"

void MatchingEngine::process_request(EngineRequest& req, 
        CallbackFunc event_handler) {
    switch (req.m_cmd)
    {

    case EngineCommand::ADD_ORDER:
        handle_add(req, event_handler);
        break;
    
    case EngineCommand::MODIFY_ORDER:
        //handle_modify(req, event_handler);
        break;

    case EngineCommand::CANCEL_ORDER:
        //handle_cancel(req, event_handler);
        break;

    default:
        break;
    }
}

void MatchingEngine::handle_add(EngineRequest& req, 
    MatchingEngine::CallbackFunc event_handler) {
    
    // make sure order is valid?

    std::cout << "processing request\n";
    auto& order = req.m_order;

    if(order.m_side == Side::BUY) {

        if(order.m_type == Type::LIMIT) {
            assert(order.m_remaining_quantity > 0);
            std::cout << "processing LIMIT BUY Order\n";

            std::vector<double> prices_to_remove;

            for(auto& [price, order_list] : m_orderbook.asks) {

                if(price <= order.m_price) {
                    int completed_orders{};

                    for(auto& resting_order : order_list) {
                        // make trade
                        
                        auto buy_size = std::min(order.m_remaining_quantity, 
                            resting_order.m_remaining_quantity);

                        order.m_remaining_quantity -= buy_size;
                        resting_order.m_remaining_quantity -= buy_size;

                        EngineResponse res {
                            Trade{resting_order.m_order_id,
                                    order.m_order_id,
                                    price,
                                    buy_size},
                            EventType::TRADE_EXECUTED,
                            req.client_id
                        };
                        event_handler(res);

                        if(resting_order.m_remaining_quantity == 0) {
                            ++completed_orders;
                        }

                        if(order.m_remaining_quantity == 0) {
                            break;
                        }
                    }

                    
                    for(int i{}; i < completed_orders; ++i) {
                        assert(!order_list.empty());
                        if(!order_list.empty()) order_list.pop_front();
                    }

                    if(order_list.empty()) prices_to_remove.push_back(price);

                    if(order.m_remaining_quantity == 0) {
                        // send filled ack
                        EngineResponse res {
                            order,
                            EventType::ACK,
                            req.client_id
                        };
                        event_handler(res);
                        break;
                    }
                }
            }

            for(auto price : prices_to_remove) {
                m_orderbook.asks.erase(price);
            }

            if(order.m_remaining_quantity > 0) { // need to rest order
                m_orderbook.bids[order.m_price].push_back(order);
                EngineResponse res {
                            order,
                            EventType::ACK,
                            req.client_id
                        };
                event_handler(res);
            }
        } else if(order.m_type == Type::MARKET) {

        }
    } else if(order.m_side == Side::SELL) {

    }
}