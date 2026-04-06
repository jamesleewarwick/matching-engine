# Matching Engine

Low-latency order book matching engine built in C++20. Lock-free MPSC queue for concurrent order ingestion, single-threaded deterministic matching core, memory-mapped trade logging.

## Performance

Benchmarked on Linux (g++, -O2, WSL2). Latency measured with rdtsc.

| Metric | Value |
|---|---|
| Insert + match throughput | ~9M orders/sec |
| Insert + match median latency | 160 cycles |
| Insert + match p99 latency | 472 cycles |
| Insert + match p99.9 latency | 2,504 cycles |
| Cancel throughput | ~15M cancels/sec |
| Cancel median latency | 96 cycles |
| Cancel p99 latency | 356 cycles |
| 4-producer ingestion throughput | ~4.2M orders/sec |
| 8-producer ingestion throughput | ~2.9M orders/sec |

Multi-threaded throughput is consumer-bound by design. The matching core stays single-threaded for determinism, the MPSC queue exists to decouple concurrent ingestion from matching, not to parallelise the order book.

## Design

### Order Book

Each side (bid/ask) is a std::map keyed by price, giving sorted price levels with O(log n) insertion. Each price level holds a doubly-linked list of orders in FIFO arrival order for time priority. A separate std::unordered_map indexes orders by ID for O(1) cancel lookup.

Prices are represented as integers (fixed-point) to avoid floating-point comparison issues in matching.

### Matching

Price-time priority. An incoming buy walks the ask side from lowest price up; an incoming sell walks the bid side from highest price down. Orders fill against resting liquidity at each level until the incoming quantity is exhausted or no more prices cross. Partial fills are supported, a partially filled incoming order rests on the book with its remaining quantity.

### MPSC Queue

Bounded ring buffer with 64k slots. Each slot contains an atomic sequence number used for coordination between producers and the single consumer. Producers claim slots via CAS (compare-and-swap) on a shared head index, write order data, then advance the slot's sequence number with a release store. The consumer reads slots sequentially, checking each slot's sequence number with an acquire load before consuming.

Slots are cache-line aligned (alignas(64)) to prevent false sharing between adjacent producer/consumer operations. The buffer is heap-allocated via std::unique_ptr to avoid stack overflow at large capacities.

### Memory Pool

Pre-allocated freelist of Order nodes backed by a std::vector. Allocation and deallocation are O(1) pointer swaps. No heap allocation occurs on the hot path after initialisation and all order nodes come from the pool.

### Trade Logger

Trades are written to disk via memory-mapped I/O (mmap on Linux, CreateFileMapping on Windows). Each trade is a fixed-size binary struct written sequentially with memcpy into the mapped region. No syscall overhead per write.

## Building

Requires C++20, CMake 3.16+.
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Running
```
./matcher          # basic test: submits two orders, verifies matching
./benchmark        # full benchmark: insert/match, cancel, multi-threaded ingestion
```

## Project Structure
```
core/           Order, PriceLevel, OrderBook, TradeLog - data types
engine/         MarketMatcher: insert, match, place, cancel logic
queue/          MPSCQueue: lock-free bounded ring buffer
memory/         OrderPool (freelist allocator), MMapWriter (mmap trade logger)
bench/          Benchmark suite with rdtsc cycle-level latency measurement
```

## Design Decisions

**Single-threaded matching.** The order book is sequential by nature - locking it for concurrent access adds latency for no gain. Concurrency belongs at ingestion, not matching.

**std::map for price levels.** Matching walks prices in order, so sorted iteration matters. For the typical number of active levels in a liquid market, log(n) is cheap.

**Intrusive linked list per level.** O(1) cancel from any position without iterator invalidation. Pool-allocated nodes keep memory local.

**CAS-based MPSC over fetch_add.** An earlier version used fetch_add for slot claiming, but preempted producers could stall the entire queue. CAS only claims when the slot is confirmed free, which fixes the problem.
