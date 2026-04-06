#pragma once
#include <cstdint>
#include "LockFreeQueue.h"
#include "OrderBook.h"
#include "OrderPool.h"
#include "MMapWriter.h"

class MarketMatcher {
public:
    explicit MarketMatcher(size_t poolSize);

    bool insert();
    bool cancel(uint64_t id);
    bool submit(const Order& o);
    bool process();

private:
    bool place(Order& o);

    MPSCQueue queue;
    OrderBook book;
    OrderPool pool;
    MMapWriter logger;
};
