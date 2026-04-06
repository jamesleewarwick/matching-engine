#pragma once
#include <map>
#include <unordered_map>
#include "PriceLevel.h"

struct OrderBook {
    std::map<int64_t, PriceLevel> bids;
    std::map<int64_t, PriceLevel> asks;

    std::unordered_map<uint64_t, Order*> mp;
};