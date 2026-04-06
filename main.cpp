#include <iostream>
#include "engine/MarketMatcher.h"
#include "core/Order.h"

int main() {
    MarketMatcher engine(10000);

    Order buyOrder{ 100, 10, 1, buy, nullptr, nullptr };
    Order sellOrder{ 100, 5, 2, sell, nullptr, nullptr };

    engine.submit(buyOrder);
    engine.submit(sellOrder);

    int count = 0;
    while (engine.process()) {}

    return 0;
}