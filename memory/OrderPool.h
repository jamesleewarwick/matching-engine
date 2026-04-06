#pragma once
#include <vector>
#include "Order.h"

class OrderPool {
public:
    OrderPool(size_t sz);

    Order* allocate();
    void deallocate(Order* node);

private:
    std::vector<Order> mempool;
    Order* list{ nullptr };
};