#include <gtest/gtest.h>
#include "mapbased_matcher.h"

void event_handler(EngineResponse res) {

}

TEST(MapBasedMatcherTest, HandlesLimitOrderCorrectly) {
    MapBasedMatcher matcher{};

    EngineRequest sell1 {
        .m_order = Order{
            .m_price{50.50},
            .m_quantity{100},
            .m_remaining_quantity{ 100 },
            .m_side{order::Side::SELL},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    EngineRequest sell2 {
        .m_order = Order{
            .m_price{60},
            .m_quantity{ 50 },
            .m_remaining_quantity{ 50 },
            .m_side{order::Side::SELL},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    matcher.process_request(sell1, event_handler);
    matcher.process_request(sell2, event_handler);

    EngineRequest buy1 {
        .m_order = Order{
            .m_price{60},
            .m_quantity{150},
            .m_remaining_quantity{ 150 },
            .m_side{order::Side::BUY},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    std::vector<EngineResponse> responses;
    auto addtolist = [&responses](EngineResponse resp) {
        responses.push_back(resp);
    };

    matcher.process_request(buy1, addtolist);

    ASSERT_EQ(responses.size(), 3);
    EXPECT_TRUE(responses[0].m_event == EventType::TRADE_EXECUTED);
    EXPECT_TRUE(std::get<Trade>(responses[0].m_payload).m_quantity == 100);
    EXPECT_TRUE(std::get<Trade>(responses[0].m_payload).m_price == 50.50);
    EXPECT_TRUE(responses[1].m_event == EventType::TRADE_EXECUTED);
    EXPECT_TRUE(std::get<Trade>(responses[1].m_payload).m_quantity == 50);
    EXPECT_TRUE(std::get<Trade>(responses[1].m_payload).m_price == 60);
    EXPECT_TRUE(std::get<Order>(responses[2].m_payload).m_remaining_quantity == 0);

    EngineRequest sell3 {
        .m_order = Order{
            .m_price{30},
            .m_quantity{ 10 },
            .m_remaining_quantity{ 10 },
            .m_side{order::Side::SELL},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    EngineRequest sell4 {
        .m_order = Order{
            .m_price{ 50 },
            .m_quantity{ 10 },
            .m_remaining_quantity{ 10 },
            .m_side{order::Side::SELL},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    matcher.process_request(sell3, event_handler);
    matcher.process_request(sell4, event_handler);

    EngineRequest buy2 {
        .m_order = Order{
            .m_price{30},
            .m_quantity{ 20 },
            .m_remaining_quantity{ 20 },
            .m_side{order::Side::BUY},
            .m_type{order::Type::LIMIT}
        },
        .m_cmd = EngineCommand::ADD_ORDER
    };

    std::vector<EngineResponse> responses2;
    auto addtolist2 = [&responses2](EngineResponse resp) {
        responses2.push_back(resp);
    };

    matcher.process_request(buy2, addtolist2);

    ASSERT_EQ(responses2.size(), 2);
    EXPECT_TRUE(responses2[0].m_event == EventType::TRADE_EXECUTED);
    EXPECT_TRUE(std::get<Trade>(responses2[0].m_payload).m_quantity == 10);
    EXPECT_TRUE(std::get<Trade>(responses2[0].m_payload).m_price == 30 );
    EXPECT_TRUE(responses2[1].m_event == EventType::ACK);
    EXPECT_TRUE(std::get<Order>(responses2[1].m_payload).m_remaining_quantity == 10);
}