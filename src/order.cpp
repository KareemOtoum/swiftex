#include "order.h"

namespace order {
    std::atomic<uint64_t> counter {1};

    uint64_t generate_order_id() {
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
}
