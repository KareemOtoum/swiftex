#pragma once

#include "order.h"
#include "matching_engine.h"

#include <cstring>
#include <variant>
#include <string>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace server {
    constexpr int k_buffer_size { 256 };
    using BufferT = std::array<std::byte, k_buffer_size>;
}

// ---- Primitive read/write ----
template <typename T>
void write_primitive(server::BufferT &buf, size_t &offset, const T &value) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (offset + sizeof(T) > buf.size()) throw std::runtime_error("Buffer overflow");
    std::memcpy(buf.data() + offset, &value, sizeof(T));
    offset += sizeof(T);
}

template <typename T>
void read_primitive(const server::BufferT &buf, size_t &offset, T &value) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (offset + sizeof(T) > buf.size()) throw std::runtime_error("Buffer underflow");
    std::memcpy(&value, buf.data() + offset, sizeof(T));
    offset += sizeof(T);
}

void serialize_request(        server::BufferT &buf,    size_t& offset,  const EngineRequest &req);
void deserialize_request(const server::BufferT &buf,    size_t& offset,        EngineRequest &req);

void serialize_response(        server::BufferT &buf,   size_t& offset,  const EngineResponse &res);
void deserialize_response(const server::BufferT &buf,   size_t& offset,        EngineResponse &res);