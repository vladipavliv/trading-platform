/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_FLATORDERBOOK_HPP
#define HFT_SERVER_FLATORDERBOOK_HPP

#include <algorithm>
#include <map>
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
    return left.price > right.price;
  }
  static bool compareAsks(const Order &left, const Order &right) {
    return left.price < right.price;
  }

public:
  using MatchHandler = std::function<void(const OrderStatus &)>;
  using UPtr = std::unique_ptr<FlatOrderBook>;

  FlatOrderBook() = delete;
  FlatOrderBook(Ticker ticker, MatchHandler matcher)
      : mTicker{ticker}, mHandler{std::move(matcher)} {
    assert(mHandler != nullptr);
    mBids.reserve(2000);
    mAsks.reserve(2000);
  }

  void add(const Order &order) {
    mOrdersCurrent.fetch_add(1);
    if (order.action == OrderAction::Buy) {
      mBids.push_back(order);
      std::push_heap(mBids.begin(), mBids.end(), compareBids);
    } else {
      mAsks.push_back(order);
      std::push_heap(mAsks.begin(), mAsks.end(), compareAsks);
    }
    match();
    // Randomization to simulate alot of traders
    if (!mBids.empty()) {
      mBids.front().traderId++;
    }
    if (!mAsks.empty()) {
      mAsks.front().traderId++;
    }
  }

  void match() {
    while (!mBids.empty() && !mAsks.empty()) {
      Order &bestBid = mBids.front();
      Order &bestAsk = mAsks.front();
      if (bestBid.price < bestAsk.price) {
        break;
      }
      uint32_t matchQuantity = std::min(bestBid.quantity, bestAsk.quantity);
      bestBid.quantity -= matchQuantity;
      bestAsk.quantity -= matchQuantity;

      handleMatch(bestBid, matchQuantity, bestAsk.price);
      handleMatch(bestAsk, matchQuantity, bestAsk.price);

      if (bestBid.quantity == 0) {
        std::pop_heap(mBids.begin(), mBids.end(), compareBids);
        mBids.pop_back();
      }
      if (bestAsk.quantity == 0) {
        std::pop_heap(mAsks.begin(), mAsks.end(), compareAsks);
        mAsks.pop_back();
      }
    }
  }

  inline size_t ordersCount() const { return mOrdersCurrent.load(); }

private:
  void handleMatch(const Order &order, Quantity quantity, Price price) {
    OrderStatus status;
    status.id = order.id;
    status.state = (order.quantity == 0) ? OrderState::Full : OrderState::Partial;
    status.quantity = quantity;
    status.fillPrice = price;
    status.action = order.action;
    status.traderId = order.traderId;
    status.ticker = order.ticker;
    spdlog::info(utils::toString(status));
    mHandler(status);
    if (order.quantity == 0) {
      mOrdersCurrent.fetch_sub(1);
    }
  }

private:
  std::vector<Order> mBids;
  std::vector<Order> mAsks;

  std::atomic<size_t> mOrdersCurrent;
  Ticker mTicker;
  MatchHandler mHandler;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
