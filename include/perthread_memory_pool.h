#pragma once

#include <vector>
#include <mutex>
#include <functional>
#include <memory>

// called when objects are acquired from the pool
template<typename T>
struct PoolInitializer {
    static void init(T* obj) {}
};

template<typename T, size_t PoolSize = 1024>
class PerThreadMemoryPool {
public:
    using PoolPtr = std::unique_ptr<T, std::function<void(T*)>>;

    static PoolPtr acquire() {
        auto& pool = get_pool();

        T* raw_ptr{ nullptr };

        if(pool.m_stack.empty()) {
            auto obj = std::make_unique<T>();
            raw_ptr = obj.get();
            pool.m_storage.push_back(std::move(obj));
        } else {
            raw_ptr = pool.m_stack.back();
            pool.m_stack.pop_back();
        }

        // send obj to custom init function
        PoolInitializer<T>::init(raw_ptr);

        return PoolPtr(raw_ptr, [](T* ptr) { release(ptr); });
    }

    static void release(T* obj) {
        get_pool().m_stack.push_back(obj);
    }

private:
    PerThreadMemoryPool() {
        m_stack.reserve(PoolSize);
        m_storage.reserve(PoolSize);
        for(int i{}; i < PoolSize; ++i) {
            auto obj = std::make_unique<T>();
            m_stack.push_back(obj.get());
            m_storage.push_back(std::move(obj));
        }
    }

    static PerThreadMemoryPool& get_pool() {
        thread_local PerThreadMemoryPool pool{};
        return pool;
    }

    std::vector<std::unique_ptr<T>> m_storage;
    std::vector<T*> m_stack;
};
