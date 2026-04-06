#include "MarketMatcher.h"
#include "TradeLog.h"
#include <algorithm>

MarketMatcher::MarketMatcher(size_t poolSize)
    : pool(poolSize), logger("trades.bin", 1 << 20) {
}

bool MarketMatcher::submit(const Order& o) {
    return queue.producer(o);
}

bool MarketMatcher::process() {
    return insert();
}

bool MarketMatcher::insert() {
    Order o;
    if (!queue.consumer(o)) return false;
    if (book.orders.count(o.orderId)) return false;

    auto& match = (o.side == buy) ? book.asks : book.bids;

    if (match.empty()) return place(o);

    while (o.quantity > 0 && !match.empty()) {
        auto best = (o.side == buy) ? match.begin() : std::prev(match.end());
        PriceLevel& level = best->second;

        if (!level.head) {
            match.erase(best);
            continue;
        }

        if (o.side == buy && best->first > o.price) break;
        if (o.side == sell && best->first < o.price) break;

        int64_t& restingQty = level.head->quantity;
        int64_t fillQty = std::min(restingQty, o.quantity);

        o.quantity -= fillQty;
        restingQty -= fillQty;

        TradeLog log{ level.head->orderId, o.orderId, best->first, fillQty };
        logger.write(&log, sizeof(log));

        if (restingQty == 0) {
            Order* filled = level.head;
            level.head = level.head->next;
            if (level.head) level.head->prev = nullptr;
            else level.tail = nullptr;

            book.orders.erase(filled->orderId);
            pool.deallocate(filled);

            if (!level.head) match.erase(best);
        }
    }

    if (o.quantity > 0) place(o);
    return true;
}

bool MarketMatcher::place(Order& o) {
    auto& levels = (o.side == buy) ? book.bids : book.asks;
    PriceLevel& level = levels[o.price];

    Order* node = pool.allocate();
    if (!node) return false;

    *node = o;
    node->next = nullptr;
    node->prev = nullptr;

    if (!level.tail) {
        level.head = node;
        level.tail = node;
    }
    else {
        level.tail->next = node;
        node->prev = level.tail;
        level.tail = node;
    }

    book.orders[o.orderId] = node;
    return true;
}

bool MarketMatcher::cancel(uint64_t id) {
    auto it = book.orders.find(id);
    if (it == book.orders.end()) return false;

    Order* target = it->second;
    auto& levels = (target->side == buy) ? book.bids : book.asks;
    auto lvlIt = levels.find(target->price);
    if (lvlIt == levels.end()) return false;

    PriceLevel& level = lvlIt->second;

    if (target->prev) target->prev->next = target->next;
    else level.head = target->next;

    if (target->next) target->next->prev = target->prev;
    else level.tail = target->prev;

    // clean up empty level
    if (!level.head) levels.erase(lvlIt);

    book.orders.erase(it);
    pool.deallocate(target);
    return true;
}