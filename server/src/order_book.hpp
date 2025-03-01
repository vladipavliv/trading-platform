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
  FlatOrderBook(FlatOrderBook &&) noexcept = default;
  FlatOrderBook &operator=(FlatOrderBook &&) noexcept = default;

  ~FlatOrderBook() = default;

  void add(const PoolPtr<Order> &order) {
    if (order->action == OrderAction::Buy) {
      mBids.push_back(*order);
      std::push_heap(mBids.begin(), mBids.end(), compareBids);
    } else {
      mAsks.push_back(*order);
      std::push_heap(mAsks.begin(), mAsks.end(), compareAsks);
    }
    mLastAdded.insert(order->id); // Randomizator
  }

  std::vector<PoolPtr<OrderStatus>> match() {
    std::vector<PoolPtr<OrderStatus>> matches;
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
  PoolPtr<OrderStatus> handleMatch(const Order &order, Quantity quantity, Price price) {
    auto status = PoolPtr<OrderStatus>::create();
    status->id = order.id;
    status->state = (order.quantity == 0) ? OrderState::Full : OrderState::Partial;
    status->quantity = quantity;
    status->fillPrice = price;
    status->action = order.action;
    status->traderId = order.traderId;
    status->ticker = order.ticker;
    spdlog::trace([&status] { return utils::toString(*status); }());
    return status;
  }

private:
  std::vector<Order> mBids;
  std::vector<Order> mAsks;
  std::set<OrderId> mLastAdded;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
