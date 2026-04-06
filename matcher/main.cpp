#include <iostream>
#include "engine/MarketMatcher.h"
#include "core/Order.h"

int main() {
    std::cout << "Starting engine" << std::endl;
    MarketMatcher engine(10000);
    std::cout << "Engine created" << std::endl;

    Order buyOrder{ 100, 10, 1, buy, nullptr, nullptr };
    Order sellOrder{ 100, 5, 2, sell, nullptr, nullptr };

    engine.submit(buyOrder);
    engine.submit(sellOrder);

    int count = 0;
    while (engine.process()) {
        std::cout << "Processed: " << ++count << std::endl;
    }

    std::cout << "Done\n";
    return 0;
}