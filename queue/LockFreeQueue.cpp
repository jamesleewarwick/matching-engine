#include "LockFreeQueue.h"
#include <cstring>
#include <new>

std::unique_ptr<Slot[], MPSCQueue::MunmapDeleter> MPSCQueue::allocateBuffer() {
    constexpr size_t size = capacity * sizeof(Slot);

    // Try huge pages first (2MB pages reduce TLB pressure on ring buffer)
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    // Fall back to regular pages if huge pages unavailable
    if (ptr == MAP_FAILED) {
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    if (ptr == MAP_FAILED)
        throw std::runtime_error("mmap failed for ring buffer");

    memset(ptr, 0, size);

    return std::unique_ptr<Slot[], MunmapDeleter>(
        static_cast<Slot*>(ptr), MunmapDeleter{ size });
}

MPSCQueue::MPSCQueue() : buffer(allocateBuffer()) {
    for (size_t i = 0; i < capacity; i++) {
        new (&buffer[i]) Slot{};
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
        }
        else if (diff < 0) {
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