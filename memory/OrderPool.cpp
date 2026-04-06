#include "OrderPool.h"

OrderPool::OrderPool(size_t sz) : mempool(sz) {
    for (size_t i = 0; i < sz - 1; i++) {
        mempool[i].next = &mempool[i + 1];
    }
    mempool[sz - 1].next = nullptr;
    list = &mempool[0];
}

Order* OrderPool::allocate() {
    if (!list) return nullptr;
    Order* node = list;
    list = list->next;
    return node;
}

void OrderPool::deallocate(Order* node) {
    node->next = list;
    list = node;
}