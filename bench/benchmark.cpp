// rdtsc header differs between MSVC and GCC/Clang
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#include <atomic>
#include "MarketMatcher.h"

void benchSingleThreaded(size_t n) {
    MarketMatcher engine(n + 1000);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int64_t> priceDist(9000, 11000);
    std::uniform_int_distribution<int64_t> qtyDist(1, 100);

    std::vector<Order> orders(n);
    for (size_t i = 0; i < n; i++) {
        orders[i] = {priceDist(rng), qtyDist(rng), i + 1,
                     (i % 2 == 0) ? buy : sell, nullptr, nullptr};
    }

    std::vector<uint64_t> cycles;
    cycles.reserve(n);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < n; i++) {
        engine.submit(orders[i]);
        uint64_t t1 = __rdtsc();
        engine.process();
        uint64_t t2 = __rdtsc();
        cycles.push_back(t2 - t1);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count() / 1e6;

    std::sort(cycles.begin(), cycles.end());

    std::cout << "=== Single-Threaded ===\n"
              << "Orders:     " << n << "\n"
              << "Time:       " << secs << "s\n"
              << "Throughput: " << (size_t)(n / secs) << " orders/sec\n"
              << "Median:     " << cycles[n / 2] << " cycles\n"
              << "p99:        " << cycles[(size_t)(n * 0.99)] << " cycles\n"
              << "p99.9:      " << cycles[(size_t)(n * 0.999)] << " cycles\n\n";
}

void benchCancel(size_t n) {
    MarketMatcher engine(n + 1000);

    // insert non-matching orders: all buys at unique prices so nothing fills
    for (size_t i = 0; i < n; i++) {
        Order o{(int64_t)(i + 1), 100, i + 1, buy, nullptr, nullptr};
        engine.submit(o);
        engine.process();
    }

    std::vector<uint64_t> cycles;
    cycles.reserve(n);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < n; i++) {
        uint64_t t1 = __rdtsc();
        engine.cancel(i + 1);
        uint64_t t2 = __rdtsc();
        cycles.push_back(t2 - t1);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count() / 1e6;

    std::sort(cycles.begin(), cycles.end());

    std::cout << "=== Cancel Benchmark ===\n"
              << "Cancels:    " << n << "\n"
              << "Time:       " << secs << "s\n"
              << "Throughput: " << (size_t)(n / secs) << " cancels/sec\n"
              << "Median:     " << cycles[n / 2] << " cycles\n"
              << "p99:        " << cycles[(size_t)(n * 0.99)] << " cycles\n"
              << "p99.9:      " << cycles[(size_t)(n * 0.999)] << " cycles\n\n";
}

void benchMultiThreaded(size_t n, int nProducers) {
    MarketMatcher engine(n + 1000);
    size_t perProducer = n / nProducers;

    std::atomic<bool> done{false};
    size_t processed = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread consumer([&]() {
        while (!done.load(std::memory_order_relaxed) || engine.process()) {
            if (!engine.process()) {
                std::this_thread::yield();
            } else {
                processed++;
            }
        }
        while (engine.process()) processed++;
    });

    std::vector<std::thread> producers;
    for (int t = 0; t < nProducers; t++) {
        producers.emplace_back([&engine, t, perProducer]() {
            std::mt19937 rng(42 + t);
            std::uniform_int_distribution<int64_t> priceDist(9000, 11000);
            std::uniform_int_distribution<int64_t> qtyDist(1, 100);

            for (size_t i = 0; i < perProducer; i++) {
                Order o{priceDist(rng), qtyDist(rng),
                        (uint64_t)(t * perProducer + i + 1),
                        (i % 2 == 0) ? buy : sell, nullptr, nullptr};

                while (!engine.submit(o))
                    std::this_thread::yield();
            }
        });
    }

    for (auto& t : producers) t.join();
    done.store(true, std::memory_order_relaxed);
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count() / 1e6;

    std::cout << "=== Multi-Threaded (" << nProducers << " producers) ===\n"
              << "Processed:  " << processed << "\n"
              << "Time:       " << secs << "s\n"
              << "Throughput: " << (size_t)(processed / secs) << " orders/sec\n\n";
}

int main() {
    constexpr size_t N = 1'000'000;
    benchSingleThreaded(N);
    benchCancel(N);
    benchMultiThreaded(N, 4);
    benchMultiThreaded(N, 8);
}
