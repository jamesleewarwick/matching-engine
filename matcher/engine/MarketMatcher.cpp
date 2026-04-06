#include "MarketMatcher.h"
#include <algorithm>
#include "TradeLog.h"
#include <iostream>

MarketMatcher::MarketMatcher(size_t poolSize)
	: pool(poolSize), logger("trades.bin", 1 << 20) {
} // Should be 1MB
bool MarketMatcher::submit(const Order& o) {
	return queue.producer(o);
}

bool MarketMatcher::process() {
	return insert();
}

bool MarketMatcher::insert() {
	Order o;
	if (!queue.consumer(o)) return false;

	auto it = book.mp.find(o.orderId);
	if (it != book.mp.end()) return false; // Already in unordered_map

	std::map<int64_t, PriceLevel>& match = (o.side == buy) ? book.asks : book.bids;

	if (match.empty()) {
		return place(o);
	}
	while (o.quantity > 0 && !match.empty()) {
		auto itMap = (o.side == buy) ? match.begin() : std::prev(match.end());
		PriceLevel& level = itMap->second;
		if (!level.head) {
			match.erase(itMap);
			continue;
		}
		if (o.side == buy && itMap->first > o.price) break; // Sell price is too low cannot find a matching order
		else if (o.side == sell && itMap->first < o.price) break; // Buy price is too high

		int64_t& matchQ = level.head->quantity;
		int64_t tradeAmount = std::min(matchQ, o.quantity);

		o.quantity -= tradeAmount;
		matchQ -= tradeAmount;
		TradeLog log{
			level.head->orderId,
			o.orderId,
			itMap->first,
			tradeAmount
		};
		logger.write(&log, sizeof(log));

		if (matchQ == 0) {
			// Unlink from structure
			Order* to_del = level.head;
			level.head = level.head->next;
			if (level.head) level.head->prev = nullptr;
			else level.tail = nullptr;

			// remove from the unordered_map
			book.mp.erase(to_del->orderId);

			// Deallocate the node allocated for this
			pool.deallocate(to_del);
			if (!level.head) match.erase(itMap);
		}
	}
	if (o.quantity > 0) place(o);
	return true;
}

bool MarketMatcher::place(Order& o) {
	// So i need to place this new order I have on the match side, but we dont have that information so that is our first job
	std::map<int64_t, PriceLevel>& place = (o.side == buy) ? book.bids : book.asks;

	// So we have the map what now?
	PriceLevel& level = place[o.price]; // Places an entry on map if no price level here yet good!
	Order* newNode = pool.allocate();
	if (!newNode) return false; // Pool allocation failed
	*newNode = o;
	newNode->next = nullptr;
	newNode->prev = nullptr;
	// So the idea is old tail moves back one piece
	if (!level.tail) {// This means also no head so we need to place a new element in this empty level
		level.head = newNode;
		level.tail = newNode;
	}
	else {
		Order* oldTail = level.tail;
		level.tail = newNode; // overwrite old with new tail value
		oldTail->next = newNode;
		newNode->prev = oldTail;
	}
	book.mp[o.orderId] = newNode;
	return true;
}

bool MarketMatcher::cancel(uint64_t id) {
	auto it = book.mp.find(id);
	if (it == book.mp.end()) return false;

	Order* node_del = it->second;
	auto& place = (node_del->side == buy) ? book.bids : book.asks;
	auto lvlIt = place.find(node_del->price);
	if (lvlIt == place.end()) return false; // defensive

	PriceLevel& level = lvlIt->second;

	// unlink node_del from the price level list
	if (node_del->prev) node_del->prev->next = node_del->next;
	else level.head = node_del->next;

	if (node_del->next) node_del->next->prev = node_del->prev;
	else level.tail = node_del->prev;

	// if price level emptied, erase it from the map using the iterator
	if (!level.head) place.erase(lvlIt);

	// remove from id map and return the node to pool
	book.mp.erase(it);
	pool.deallocate(node_del);
	return true;
}