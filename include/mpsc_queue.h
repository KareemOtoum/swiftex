#pragma once

#include <atomic>
#include <memory>

template <typename T>
class MPSCQueue {
public:
    MPSCQueue() {
        Node* dummy = new Node(); // dummy node
        m_head.store(dummy);
        m_tail = dummy;
    }

    ~MPSCQueue() {
        while (Node* old = m_tail) {
            m_tail = old->next;
            delete old;
        }
    }

    void enqueue(const T& value) {
        Node* node = new Node(value);
        Node* prev_m_head = m_head.exchange(node);
        prev_m_head->next = node;
    }

    bool dequeue(T& result) {
        Node* next = m_tail->next;
        if (next) {
            result = std::move(next->data);
            delete m_tail;
            m_tail = next;
            return true;
        }
        return false;
    }

    bool empty() const {
        return m_tail->next == nullptr;
    }

private:
    struct Node {
        T data;
        Node* next = nullptr;
        Node() = default;
        explicit Node(const T& val) : data(val) {}
    };

    std::atomic<Node*> m_head;
    Node* m_tail;
};
