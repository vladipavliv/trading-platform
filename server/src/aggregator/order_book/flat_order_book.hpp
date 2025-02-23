/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_FLATORDERBOOK_HPP
#define HFT_SERVER_FLATORDERBOOK_HPP

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "market_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

class FlatOrderBook {
  static bool compareBids(const Order &left, const Order &right) {
    return left.price < right.price;
  }
  static bool compareAsks(const Order &left, const Order &right) {
    return left.price > right.price;
  }

public:
  using UPtr = std::unique_ptr<FlatOrderBook>;

  FlatOrderBook() {
    mBids.reserve(500);
    mAsks.reserve(500);
  }
  ~FlatOrderBook() = default;

  inline bool acquire() { return !mBusy.test_and_set(std::memory_order_acq_rel); }
  inline void release() { mBusy.clear(std::memory_order_acq_rel); }
  inline size_t ordersCount() const { return mOrdersCurrent.load(std::memory_order_relaxed); }

  void add(Span<Order> orders) {
    for (auto &order : orders) {
      mOrdersCurrent.fetch_add(1);
      if (order.action == OrderAction::Buy) {
        mBids.push_back(order);
        std::push_heap(mBids.begin(), mBids.end(), compareBids);
      } else {
        mAsks.push_back(order);
        std::push_heap(mAsks.begin(), mAsks.end(), compareAsks);
      }
      mLastAdded.insert(order.id); // Randomizator
    }
  }

  std::vector<OrderStatus> match() {
    std::vector<OrderStatus> matches;
    matches.reserve(10);

    while (!mBids.empty() && !mAsks.empty()) {
      Order &bestBid = mBids.front();
      Order &bestAsk = mAsks.front();
      if (bestBid.price < bestAsk.price) {
        break;
      }
      auto quantity = std::min(bestBid.quantity, bestAsk.quantity);
      bestBid.quantity -= quantity;
      bestAsk.quantity -= quantity;

      if (mLastAdded.contains(bestBid.id)) {
        matches.emplace_back(handleMatch(bestBid, quantity, bestAsk.price));
      }
      if (mLastAdded.contains(bestAsk.id)) {
        matches.emplace_back(handleMatch(bestAsk, quantity, bestAsk.price));
      }

      if (bestBid.quantity == 0) {
        std::pop_heap(mBids.begin(), mBids.end(), compareBids);
        mBids.pop_back();
      }
      if (bestAsk.quantity == 0) {
        std::pop_heap(mAsks.begin(), mAsks.end(), compareAsks);
        mAsks.pop_back();
      }
    }
    mLastAdded.clear();
    return matches;
  }

private:
  OrderStatus handleMatch(const Order &order, Quantity quantity, Price price) {
    OrderStatus status;
    status.id = order.id;
    status.state = (order.quantity == 0) ? OrderState::Full : OrderState::Partial;
    status.quantity = quantity;
    status.fillPrice = price;
    status.action = order.action;
    status.traderId = order.traderId;
    status.ticker = order.ticker;
    spdlog::trace(utils::toString(status));
    if (order.quantity == 0) {
      mOrdersCurrent.fetch_sub(1);
    }
    return status;
  }

private:
  std::vector<Order> mBids;
  std::vector<Order> mAsks;

  std::set<OrderId> mLastAdded;
  std::atomic_flag mBusy = ATOMIC_FLAG_INIT;

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> mOrdersCurrent;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
