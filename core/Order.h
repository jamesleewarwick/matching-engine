#pragma once
#include <cstdint>

enum Side {
    buy,
    sell
};

struct Order {
    int64_t price;
    int64_t quantity;
    uint64_t orderId;
    Side side;

    Order* next;
    Order* prev;
};