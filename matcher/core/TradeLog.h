#pragma once
#include <cstdint>

struct TradeLog {
    uint64_t makerOrderId;
    uint64_t takerOrderId;
    int64_t price;
    int64_t quantity;
};