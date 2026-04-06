#define NOMINMAX
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#include <atomic>
#include "MarketMatcher.h"

void runSingleThreaded(size_t numOrders) {
    MarketMatcher engine(numOrders + 1000);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int64_t> priceDist(9000, 11000);
    std::uniform_int_distribution<int64_t> qtyDist(1, 100);

    std::vector<Order> orders(numOrders);
    for (size_t i = 0; i < numOrders; i++) {
        orders[i].price = priceDist(rng);
        orders[i].quantity = qtyDist(rng);
        orders[i].orderId = i + 1;
        orders[i].side = (i % 2 == 0) ? buy : sell;
        orders[i].next = nullptr;
        orders[i].prev = nullptr;
    }

    std::vector<double> latencies;
    latencies.reserve(numOrders);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < numOrders; i++) {
        engine.submit(orders[i]);
        auto t1 = std::chrono::high_resolution_clock::now();
        engine.process();
        auto t2 = std::chrono::high_resolution_clock::now();
        latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1e6;

    std::sort(latencies.begin(), latencies.end());

    std::cout << "=== Single-Threaded Benchmark ===" << std::endl;
    std::cout << "Orders processed: " << numOrders << std::endl;
    std::cout << "Total time: " << totalSec << "s" << std::endl;
    std::cout << "Throughput: " << (size_t)(numOrders / totalSec) << " orders/sec" << std::endl;
    std::cout << "Median latency: " << latencies[numOrders / 2] << "ns" << std::endl;
    std::cout << "p99 latency: " << latencies[(size_t)(numOrders * 0.99)] << "ns" << std::endl;
    std::cout << "p99.9 latency: " << latencies[(size_t)(numOrders * 0.999)] << "ns" << std::endl;
    std::cout << std::endl;
}

void runMultiThreaded(size_t numOrders, int numProducers) {
    std::cout << "MT: Starting " << numProducers << " producer test..." << std::endl;
    MarketMatcher engine(numOrders + 1000);

    size_t ordersPerProducer = numOrders / numProducers;
    std::atomic<size_t> totalSubmitted{ 0 };

    // Phase 1: producers submit concurrently, consumer drains concurrently
    std::atomic<bool> done{ false };
    size_t processed = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Consumer thread
    std::thread consumer([&]() {
        while (!done.load(std::memory_order_relaxed) || engine.process()) {
            if (!engine.process()) {
                std::this_thread::yield();
            }
            else {
                processed++;
            }
        }
        // Final drain
        while (engine.process()) {
            processed++;
        }
        });

    // Producer threads
    std::vector<std::thread> producers;
    for (int t = 0; t < numProducers; t++) {
        producers.emplace_back([&engine, t, ordersPerProducer, &totalSubmitted]() {
            std::mt19937 rng(42 + t);
            std::uniform_int_distribution<int64_t> priceDist(9000, 11000);
            std::uniform_int_distribution<int64_t> qtyDist(1, 100);

            for (size_t i = 0; i < ordersPerProducer; i++) {
                Order o;
                o.price = priceDist(rng);
                o.quantity = qtyDist(rng);
                o.orderId = t * ordersPerProducer + i + 1;
                o.side = (i % 2 == 0) ? buy : sell;
                o.next = nullptr;
                o.prev = nullptr;

                while (!engine.submit(o)) {
                    std::this_thread::yield();
                }
                totalSubmitted.fetch_add(1, std::memory_order_relaxed);
            }
            });
    }

    for (auto& t : producers) {
        t.join();
    }
    done.store(true, std::memory_order_relaxed);
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1e6;

    std::cout << "=== Multi-Threaded Benchmark (" << numProducers << " producers) ===" << std::endl;
    std::cout << "Orders submitted: " << totalSubmitted.load() << std::endl;
    std::cout << "Orders processed: " << processed << std::endl;
    std::cout << "Total time: " << totalSec << "s" << std::endl;
    std::cout << "Throughput: " << (size_t)(processed / totalSec) << " orders/sec" << std::endl;
    std::cout << std::endl;
}

int main() {
    const size_t NUM_ORDERS = 1000000;

    runSingleThreaded(NUM_ORDERS);
    runMultiThreaded(NUM_ORDERS, 4);
    runMultiThreaded(NUM_ORDERS, 8);

    return 0;
}
