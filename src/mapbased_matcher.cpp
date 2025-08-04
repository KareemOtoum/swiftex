#include "mapbased_matcher.h"

void MapBasedMatcher::run_impl() {
    while (server::g_running) {
        
    }
}

void MapBasedMatcher::process_request_impl(EngineRequest& req, 
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

void MapBasedMatcher::handle_add(EngineRequest& req, 
    CallbackFunc event_handler) {
    
    // make sure order is valid?

    auto& order = req.m_order;

    if(order.m_type == order::Type::LIMIT) {

        if(order.m_side == order::Side::BUY) {
            handle_limit_buy(req, event_handler);
        } else if(order.m_side == order::Side::SELL) {
            handle_limit_sell(req, event_handler);
        }
    } else if(order.m_type == order::Type::MARKET) {

        if(order.m_side == order::Side::BUY) {
            handle_market_buy(req, event_handler);
        } else if(order.m_side == order::Side::SELL) {
            handle_market_sell(req, event_handler);
        }
    }
}

void MapBasedMatcher::handle_limit_buy(EngineRequest& req, 
    CallbackFunc event_handler) {

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

            EngineResponse res{
                .m_payload = Trade{resting_order.m_id,
                      order.m_id,
                      price,
                      buy_size},
                .client_id = req.client_id,
                .m_event = EventType::TRADE_EXECUTED};
            event_handler(res);

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
            EngineResponse res{
                order,
                req.client_id,
                EventType::ACK};
            event_handler(res);
            break;
        }
    }

    std::erase_if(m_orderbook.asks, 
        [](const auto& pair) { return pair.second.empty(); });

    // need to rest order
    if (order.m_remaining_quantity > 0) {
        m_orderbook.bids[order.m_price].push_back(order);
    }
}

void MapBasedMatcher::handle_limit_sell(EngineRequest& req, 
    MapBasedMatcher::CallbackFunc event_handler) {

    auto& order = req.m_order;

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

            EngineResponse res{
                Trade{resting_order.m_id,
                      order.m_id,
                      price,
                      buy_size},
                      req.client_id,
                EventType::TRADE_EXECUTED};
            event_handler(res);

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
            EngineResponse res{
                order,
                req.client_id,
                EventType::ACK};
            event_handler(res);
            break;
        }
    }

    std::erase_if(m_orderbook.bids, 
        [](const auto& pair) { return pair.second.empty(); });

    // need to rest order
    if (order.m_remaining_quantity > 0) {
        m_orderbook.asks[order.m_price].push_back(order);
    }
}


void MapBasedMatcher::handle_market_buy(EngineRequest& req,
    CallbackFunc event_handler) {

    auto& order = req.m_order;
    assert(order.m_remaining_quantity > 0);

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

            EngineResponse res{
                Trade{resting_order.m_id,
                      order.m_id,
                      price,
                      buy_size},
                      req.client_id,
                EventType::TRADE_EXECUTED};
            event_handler(res);

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
            EngineResponse res{
                order,
                req.client_id,
                EventType::ACK};
            event_handler(res);
            break;
        }
    }

    std::erase_if(m_orderbook.asks, 
        [](const auto& pair) { return pair.second.empty(); });

    // CANNOT rest market order
    if (order.m_remaining_quantity > 0) {
        EngineResponse res{
            order,
            req.client_id,
            EventType::ACK};
        event_handler(res);
    }
}
void MapBasedMatcher::handle_market_sell(EngineRequest& req,
    MapBasedMatcher::CallbackFunc event_handler) {

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

            EngineResponse res{
                Trade{resting_order.m_id,
                      order.m_id,
                      price,
                      buy_size},
                      req.client_id,
                EventType::TRADE_EXECUTED};
            event_handler(res);

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
            EngineResponse res{
                order,
                req.client_id,
                EventType::ACK};
            event_handler(res);
            break;
        }
    }

    std::erase_if(m_orderbook.bids, 
        [](const auto& pair) { return pair.second.empty(); });

    // cannot rest market order
    if (order.m_remaining_quantity > 0) {
        EngineResponse res{
            order,
            req.client_id,
            EventType::ACK};
        event_handler(res);
    }
}