#pragma once

#include <atomic>
#include <cstddef>
#include <sys/mman.h>
#include <stdexcept>
#include <memory>
#include "Order.h"

constexpr size_t capacity = 65536;

struct alignas(64) Slot {
    std::atomic<size_t> seq;
    Order data;
};

class MPSCQueue {
public:
    MPSCQueue();

    bool producer(const Order& o);
    bool consumer(Order& out);

private:
    struct MunmapDeleter {
        size_t size;
        void operator()(Slot* ptr) const noexcept {
            if (ptr && ptr != MAP_FAILED)
                munmap(ptr, size);
        }
    };

    static std::unique_ptr<Slot[], MunmapDeleter> allocateBuffer();

    std::unique_ptr<Slot[], MunmapDeleter> buffer;

    alignas(64) std::atomic<size_t> head{ 0 }; // multi-producer
    alignas(64) size_t tail { 0 };               // single-consumer
};
