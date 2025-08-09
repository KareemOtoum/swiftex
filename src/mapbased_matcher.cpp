#include "mapbased_matcher.h"
#include "server.h"

void MapBasedMatcher::run_impl() {
    EngineRequest* req{};
    EngineResponse* returned_res{};
    while (server::g_running) {

        // handle new request
        if(m_request_queue.pop(req)) {
            // process request
            int worker_id{ req->m_worker_id };
            process_request_impl(*req);            
        }

        // handle returned response obj
        if(m_response_return_queue.pop(returned_res)) {
            PerThreadMemoryPool<EngineResponse>::release(returned_res);
        }
    }
}

void MapBasedMatcher::send_worker_response(
    EngineResponse* response) {
        std::cout << "sent response to worker id: " << response->m_worker_id << "\n";
    m_response_queue_list[response->m_worker_id]->push(response);
}

void MapBasedMatcher::process_request_impl(EngineRequest& req) {
    switch (req.m_cmd)
    {
    case EngineCommand::ADD_ORDER:
        handle_add(req);
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

void MapBasedMatcher::handle_add(EngineRequest& req) {
    
    // make sure order is valid?

    auto& order = req.m_order;

    if(order.m_type == order::Type::LIMIT) {

        if(order.m_side == order::Side::BUY) {
            handle_limit_buy(req);
        } else if(order.m_side == order::Side::SELL) {
            handle_limit_sell(req);
        }
    } else if(order.m_type == order::Type::MARKET) {

        if(order.m_side == order::Side::BUY) {
            handle_market_buy(req);
        } else if(order.m_side == order::Side::SELL) {
            handle_market_sell(req);
        }
    }
}

void MapBasedMatcher::handle_limit_buy(EngineRequest& req) {

    auto& order = req.m_order;
    assert(order.m_remaining_quantity > 0);

    std::cout << "processing LIMIT BUY Order\n";

    for (auto& [price, order_list] : m_orderbook.asks) {
        if (price > order.m_price) {
            break;
        }

        int completed_orders{};

        for (auto &resting_order : order_list)
        {
            // make trade

            auto buy_size = std::min(order.m_remaining_quantity,
                                     resting_order.m_remaining_quantity);

            order.m_remaining_quantity -= buy_size;
            resting_order.m_remaining_quantity -= buy_size;

            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = Trade{resting_order.m_id, 
                    order.m_id,
                    price,
                    buy_size};
            response->client_id = req.client_id;
            response->m_event = EventType::TRADE_EXECUTED;
            response->m_worker_id = req.m_worker_id;

            std::cout << "sent response to worker " << response->m_worker_id << "\n";
            send_worker_response(response);

            if (resting_order.m_remaining_quantity == 0)
            {
                ++completed_orders;
            }

            if (order.m_remaining_quantity == 0)
            {
                break;
            }
        }

        for (int i{}; i < completed_orders; ++i)
        {
            assert(!order_list.empty());
            if (!order_list.empty())
                order_list.pop_front();
        }

        if (order.m_remaining_quantity == 0) {
            // send filled ack
            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = order;
            response->client_id = req.client_id;
            response->m_event = EventType::ACK;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);
            break;
        }
    }

    std::erase_if(m_orderbook.asks, 
        [](const auto& pair) { return pair.second.empty(); });

    // need to rest order
    if (order.m_remaining_quantity > 0) {
        m_orderbook.bids[order.m_price].push_back(order);
    }

    std::cout << "exiting limit buy order\n";
    std::cout << m_orderbook;
}

void MapBasedMatcher::handle_limit_sell(
    EngineRequest& req) {

    auto& order = req.m_order;
    if (order.m_remaining_quantity <= 0) {
        std::cerr << "Rejecting order with zero remaining quantity\n";
        return;
    }

    std::cout << "processing LIMIT SELL Order\n";
    for(auto& [price, order_list] : m_orderbook.bids) {
        if(price < order.m_price) {
            break;
        }

        int completed_orders{};

        for (auto &resting_order : order_list)
        {
            // make trade

            auto buy_size = std::min(order.m_remaining_quantity,
                                     resting_order.m_remaining_quantity);

            order.m_remaining_quantity -= buy_size;
            resting_order.m_remaining_quantity -= buy_size;

            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = Trade{resting_order.m_id, 
                    order.m_id,
                    price,
                    buy_size};
            response->client_id = req.client_id;
            response->m_event = EventType::TRADE_EXECUTED;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);

            if (resting_order.m_remaining_quantity == 0)
            {
                ++completed_orders;
            }

            if (order.m_remaining_quantity == 0)
            {
                break;
            }
        }

        for (int i{}; i < completed_orders; ++i)
        {
            assert(!order_list.empty());
            if (!order_list.empty())
                order_list.pop_front();
        }

        if (order.m_remaining_quantity == 0) {
            // send filled ack
            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = order;
            response->client_id = req.client_id;
            response->m_event = EventType::ACK;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);
            break;
        }
    }

    std::erase_if(m_orderbook.bids, 
        [](const auto& pair) { return pair.second.empty(); });

    // need to rest order
    if (order.m_remaining_quantity > 0) {
        m_orderbook.asks[order.m_price].push_back(order);
    }

    std::cout << "exiting limit sell order\n";
    std::cout << m_orderbook;
}


void MapBasedMatcher::handle_market_buy(
    EngineRequest& req) {

    auto& order = req.m_order;
    
    if (order.m_remaining_quantity <= 0) {
        std::cerr << "Rejecting order with zero remaining quantity\n";
        return;
    }

    std::cout << "processing MARKET BUY Order\n";

    for (auto& [price, order_list] : m_orderbook.asks) {

        int completed_orders{};

        for (auto &resting_order : order_list)
        {
            // make trade

            auto buy_size = std::min(order.m_remaining_quantity,
                                     resting_order.m_remaining_quantity);

            order.m_remaining_quantity -= buy_size;
            resting_order.m_remaining_quantity -= buy_size;

            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = Trade{resting_order.m_id, 
                    order.m_id,
                    price,
                    buy_size};
            response->client_id = req.client_id;
            response->m_event = EventType::TRADE_EXECUTED;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);

            if (resting_order.m_remaining_quantity == 0)
            {
                ++completed_orders;
            }

            if (order.m_remaining_quantity == 0)
            {
                break;
            }
        }

        for (int i{}; i < completed_orders; ++i)
        {
            assert(!order_list.empty());
            if (!order_list.empty())
                order_list.pop_front();
        }

        if (order.m_remaining_quantity == 0) {
            // send filled ack
            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = order;
            response->client_id = req.client_id;
            response->m_event = EventType::ACK;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);
            break;
        }
    }

    std::erase_if(m_orderbook.asks, 
        [](const auto& pair) { return pair.second.empty(); });

    // CANNOT rest market order
    if (order.m_remaining_quantity > 0) {
        auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
        response->m_payload = order;
        response->client_id = req.client_id;
        response->m_event = EventType::ACK;
        response->m_worker_id = req.m_worker_id;

        send_worker_response(response);
    }
}
void MapBasedMatcher::handle_market_sell(
    EngineRequest& req) {

    auto& order = req.m_order;

    std::cout << "processing MARKET SELL Order\n";
    for(auto& [price, order_list] : m_orderbook.bids) {

        int completed_orders{};

        for (auto &resting_order : order_list)
        {
            // make trade

            auto buy_size = std::min(order.m_remaining_quantity,
                                     resting_order.m_remaining_quantity);

            order.m_remaining_quantity -= buy_size;
            resting_order.m_remaining_quantity -= buy_size;

            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = Trade{resting_order.m_id, 
                    order.m_id,
                    price,
                    buy_size};
            response->client_id = req.client_id;
            response->m_event = EventType::TRADE_EXECUTED;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);

            if (resting_order.m_remaining_quantity == 0)
            {
                ++completed_orders;
            }

            if (order.m_remaining_quantity == 0)
            {
                break;
            }
        }

        for (int i{}; i < completed_orders; ++i)
        {
            assert(!order_list.empty());
            if (!order_list.empty())
                order_list.pop_front();
        }

        if (order.m_remaining_quantity == 0) {
            // send filled ack
            auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
            response->m_payload = order;
            response->client_id = req.client_id;
            response->m_event = EventType::ACK;
            response->m_worker_id = req.m_worker_id;

            send_worker_response(response);
            break;
        }
    }

    std::erase_if(m_orderbook.bids, 
        [](const auto& pair) { return pair.second.empty(); });

    // cannot rest market order
    if (order.m_remaining_quantity > 0) {
        auto* response = PerThreadMemoryPool<EngineResponse>::acquire();
        response->m_payload = order;
        response->client_id = req.client_id;
        response->m_event = EventType::ACK;
        response->m_worker_id = req.m_worker_id;

        send_worker_response(response);
    }
}