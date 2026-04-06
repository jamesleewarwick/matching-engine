#pragma once

#include <atomic>
#include <cstddef>
#include "Order.h"
#include <memory>

constexpr size_t capacity = 65536;

struct alignas(64) Slot {
    std::atomic<size_t> seq;
    Order data;
};

class MSPCRQB {
public:
    MSPCRQB();

    bool producer(const Order& o);
    bool consumer(Order& out);

private:
    std::unique_ptr<Slot[]> buffer;

    std::atomic<size_t> head{ 0 }; // multi-producer
    size_t tail{ 0 };              // single-consumer
};