#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <future>

#include "matching_engine.h"  // for EngineRequest, EngineResponse
#include "swift_protocol.h"    // for serialize_request, deserialize_response

using namespace std;
using namespace server;


static const string host = "127.0.0.1";
static const string port = "5050"; 

int connect_to_server(const std::string& host, const std::string& port) {
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (status != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << endl;
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

void send_buy() {
    int socket = connect_to_server(host, port);
    if (socket == -1) return;

    EngineRequest req{};
    req.m_order.m_symbol = "AAPL";
    req.m_order.m_price = 150.25;
    req.m_order.m_quantity = 100;
    req.m_order.m_remaining_quantity = 100;
    req.m_order.m_side = order::Side::BUY;
    req.m_order.m_type = order::Type::LIMIT;
    req.m_cmd = EngineCommand::ADD_ORDER;

    server::BufferT buffer{};
    size_t offset = 0;
    serialize_request(buffer, offset, req);

    // Send request
    if (send(socket, buffer.data(), buffer.size(), 0) == -1) {
        perror("send");
        close(socket);
        return;
    }

    cout << "Request sent.\n";

    // Receive response
    BufferT resp_buffer{};
    ssize_t bytes_received = recv(socket, resp_buffer.data(), resp_buffer.size(), 0);
    if (bytes_received <= 0) {
        cerr << "Server closed connection or recv failed.\n";
        close(socket);
        return;
    }

    EngineResponse res{};
    offset = 0;
    deserialize_response(resp_buffer, offset, res);

    cout << "Response received:\n";
    cout << "Client ID: " << res.client_id << "\n";
    cout << "Worker ID: " << static_cast<int>(res.m_worker_id) << "\n";
    cout << "Event Type: ";
    switch (res.m_event)
    {
    case EventType::TRADE_EXECUTED:
        cout << "Trade Executed\n";
        break;
    
    case EventType::ACK:
        cout << "ACK\n";
    default:
        break;
    }
    if(std::holds_alternative<Trade>(res.m_payload)) {
        auto& trade = std::get<Trade>(res.m_payload);
        cout << "Order ID: " << trade.m_incoming_order_id << "\n";
        cout << "Price: " << trade.m_price << "\n";
        cout << "Quantity: " << trade.m_quantity<< "\n";
    }
    close(socket);
}

void send_sell() {

    int socket = connect_to_server(host, port);
    if (socket == -1) return;

    EngineRequest req{};
    req.m_order.m_symbol = "AAPL";
    req.m_order.m_price = 100.0;
    req.m_order.m_quantity = 50;
    req.m_order.m_remaining_quantity = 50;
    req.m_order.m_side = order::Side::SELL;
    req.m_order.m_type = order::Type::MARKET;
    req.m_cmd = EngineCommand::ADD_ORDER;

    server::BufferT buffer{};
    size_t offset = 0;
    serialize_request(buffer, offset, req);

    // Send request
    if (send(socket, buffer.data(), buffer.size(), 0) == -1) {
        perror("send");
        close(socket);
        return;
    }

    cout << "Request sent.\n";

    // Receive response
    BufferT resp_buffer{};
    ssize_t bytes_received = recv(socket, resp_buffer.data(), resp_buffer.size(), 0);
    if (bytes_received <= 0) {
        cerr << "Server closed connection or recv failed.\n";
        close(socket);
        return;
    }

    EngineResponse res{};
    offset = 0;
    deserialize_response(resp_buffer, offset, res);

    cout << "Response received:\n";
    cout << "Client ID: " << res.client_id << "\n";
    cout << "Worker ID: " << static_cast<int>(res.m_worker_id) << "\n";
    cout << "Event Type: ";
    switch (res.m_event)
    {
    case EventType::TRADE_EXECUTED:
        cout << "Trade Executed\n";
        break;
    
    case EventType::ACK:
        cout << "ACK\n";
    default:
        break;
    }
    if(std::holds_alternative<Trade>(res.m_payload)) {
        auto& trade = std::get<Trade>(res.m_payload);
        cout << "Order ID: " << trade.m_incoming_order_id << "\n";
        cout << "Price: " << trade.m_price << "\n";
        cout << "Quantity: " << trade.m_quantity<< "\n";
    }
    close(socket);
}


int main() {

    cout << "Connected to server.\n";


    auto sell_future = std::async(send_sell);
    auto buy_future = std::async(send_buy);
    
    return 0;
}
