#include "LockFreeQueue.h"

MPSCQueue::MPSCQueue() : buffer(new Slot[capacity]) {
    for (size_t i = 0; i < capacity; i++) {
        buffer[i].seq.store(i, std::memory_order_relaxed);
    }
}

bool MPSCQueue::producer(const Order& o) {
    while (true) {
        size_t pos = head.load(std::memory_order_relaxed);
        Slot& slot = buffer[pos % capacity];
        size_t seq = slot.seq.load(std::memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;

        if (diff == 0) {
            if (head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                slot.data = o;
                slot.seq.store(pos + 1, std::memory_order_release);
                return true;
            }
        } else if (diff < 0) {
            return false;
        }
    }
}

bool MPSCQueue::consumer(Order& out) {
    Slot& slot = buffer[tail % capacity];

    while (true) {
        size_t seq = slot.seq.load(std::memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)(tail + 1);

        if (diff == 0) break;
        if (diff < 0) return false;
    }

    out = slot.data;
    slot.seq.store(tail + capacity, std::memory_order_release);
    tail++;
    return true;
}
