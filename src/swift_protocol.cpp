#include "swift_protocol.h"

void write_string(server::BufferT &buf, size_t &offset, const std::string &str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    write_primitive(buf, offset, len);
    if (offset + len > buf.size()) throw std::runtime_error("Buffer overflow");
    std::memcpy(buf.data() + offset, str.data(), len);
    offset += len;
}

void read_string(const server::BufferT &buf, size_t &offset, std::string &str) {
    uint16_t len{};
    read_primitive(buf, offset, len);
    if (offset + len > buf.size()) throw std::runtime_error("Buffer underflow");
    str.assign(reinterpret_cast<const char*>(buf.data() + offset), len);
    offset += len;
}

void serialize_order(server::BufferT &buf, size_t &offset, const Order &o) {
    write_string(buf, offset, o.m_symbol);
    write_primitive(buf, offset, o.m_price);
    write_primitive(buf, offset, o.m_id);
    write_primitive(buf, offset, o.m_quantity);
    write_primitive(buf, offset, o.m_remaining_quantity);
    write_primitive(buf, offset, static_cast<uint8_t>(o.m_side));
    write_primitive(buf, offset, static_cast<uint8_t>(o.m_type));
}

void deserialize_order(const server::BufferT &buf, size_t &offset, Order &o) {
    read_string(buf, offset, o.m_symbol);
    read_primitive(buf, offset, o.m_price);
    read_primitive(buf, offset, o.m_id);
    read_primitive(buf, offset, o.m_quantity);
    read_primitive(buf, offset, o.m_remaining_quantity);
    uint8_t side, type;
    read_primitive(buf, offset, side);
    read_primitive(buf, offset, type);
    o.m_side = static_cast<order::Side>(side);
    o.m_type = static_cast<order::Type>(type);
}

void serialize_trade(server::BufferT &buf, size_t &offset, const Trade &t) {
    write_primitive(buf, offset, t.m_resting_order_id);
    write_primitive(buf, offset, t.m_incoming_order_id);
    write_primitive(buf, offset, t.m_price);
    write_primitive(buf, offset, t.m_quantity);
}

void deserialize_trade(const server::BufferT &buf, size_t &offset, Trade &t) {
    read_primitive(buf, offset, t.m_resting_order_id);
    read_primitive(buf, offset, t.m_incoming_order_id);
    read_primitive(buf, offset, t.m_price);
    read_primitive(buf, offset, t.m_quantity);
}

void serialize_request(server::BufferT &buf, size_t& offset, const EngineRequest &req) {
    serialize_order(buf, offset, req.m_order);
    write_primitive(buf, offset, req.client_id);
    write_primitive(buf, offset, static_cast<uint8_t>(req.m_cmd));
    write_primitive(buf, offset, req.m_worker_id);
}

void deserialize_request(const server::BufferT &buf, size_t& offset, EngineRequest &req) {
    deserialize_order(buf, offset, req.m_order);
    read_primitive(buf, offset, req.client_id);
    uint8_t cmd;
    read_primitive(buf, offset, cmd);
    req.m_cmd = static_cast<EngineCommand>(cmd);
    read_primitive(buf, offset, req.m_worker_id);
}

void serialize_response(server::BufferT &buf, size_t &offset, const EngineResponse &res) {
    uint8_t index = static_cast<uint8_t>(res.m_payload.index());
    write_primitive(buf, offset, index);

    if (index == 0) {
        serialize_trade(buf, offset, std::get<Trade>(res.m_payload));
    } else if (index == 1) {
        serialize_order(buf, offset, std::get<Order>(res.m_payload));
    } else if (index == 2) {
        write_primitive(buf, offset, std::get<uint64_t>(res.m_payload));
    }

    write_primitive(buf, offset, res.client_id);
    write_primitive(buf, offset, static_cast<uint8_t>(res.m_event));
    write_primitive(buf, offset, res.m_worker_id);
}

void deserialize_response(const server::BufferT &buf, size_t &offset, EngineResponse &res) {
    uint8_t index;
    read_primitive(buf, offset, index);

    if (index == 0) {
        Trade t;
        deserialize_trade(buf, offset, t);
        res.m_payload = t;
    } else if (index == 1) {
        Order o;
        deserialize_order(buf, offset, o);
        res.m_payload = o;
    } else if (index == 2) {
        uint64_t val;
        read_primitive(buf, offset, val);
        res.m_payload = val;
    } else {
        throw std::runtime_error("Invalid variant index");
    }

    read_primitive(buf, offset, res.client_id);
    uint8_t evt;
    read_primitive(buf, offset, evt);
    res.m_event = static_cast<EventType>(evt);
    read_primitive(buf, offset, res.m_worker_id);
}
