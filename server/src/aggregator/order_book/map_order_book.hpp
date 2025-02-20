/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MAPORDERBOOK_HPP
#define HFT_SERVER_MAPORDERBOOK_HPP

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

/**
 * @brief For simulation sake if order not matched immediately TraderId gets changed
 * to simulate lots of traders and not polute client side statistics with filled orders
 * that were placed 5 seconds ago
 */
class MapOrderBook {
public:
  using MatchHandler = std::function<void(const OrderStatus &)>;
  using UPtr = std::unique_ptr<MapOrderBook>;

  MapOrderBook() = delete;
  MapOrderBook(Ticker ticker, MatchHandler matcher)
      : mTicker{ticker}, mHandler{std::move(matcher)} {
    assert(mHandler != nullptr);
  }

  void add(const Order &order) {
    mOrdersCurrent.fetch_add(1);
    if (order.action == OrderAction::Buy) {
      if (mBids.size() > ORDER_BOOK_LIMIT) {
        spdlog::error("Order limit reached for {}", utils::toStrView(mTicker));
        return;
      }
      auto &priceBids = mBids[order.price];
      if (priceBids.empty()) { // Was just created
        priceBids.reserve(ORDER_BOOK_LIMIT);
      }
      priceBids.push_back(order);
    } else {
      if (mAsks.size() > ORDER_BOOK_LIMIT) {
        spdlog::error("Order limit reached for {}", utils::toStrView(mTicker));
        return;
      }
      auto &priceBids = mAsks[order.price];
      if (priceBids.empty()) {
        priceBids.reserve(ORDER_BOOK_LIMIT);
      }
      priceBids.push_back(order);
    }
    match();
    if (mBids.contains(order.price)) {
      mBids[order.price].back().traderId++;
    }
    if (mAsks.contains(order.price)) {
      mAsks[order.price].back().traderId++;
    }
  }

  void match() {
    while (!mBids.empty() && !mAsks.empty()) {
      auto &bestBids = mBids.begin()->second;
      auto &bestAsks = mAsks.begin()->second;

      if (bestBids.back().price < bestAsks.back().price) {
        break;
      }
      auto &bestBid = bestBids.back();
      auto &bestAsk = bestAsks.back();

      int quantity = std::min(bestBid.quantity, bestAsk.quantity);

      bestBid.quantity -= quantity;
      bestAsk.quantity -= quantity;

      handleMatch(bestBid, quantity, bestAsk.price);
      handleMatch(bestAsk, quantity, bestAsk.price);

      if (bestBid.quantity == 0) {
        bestBids.pop_back();
        if (bestBids.empty()) {
          mBids.erase(mBids.begin());
        }
      }
      if (bestAsk.quantity == 0) {
        bestAsks.pop_back();
        if (bestAsks.empty()) {
          mAsks.erase(mAsks.begin());
        }
      }
    }
  }

  inline size_t ordersCount() const { return mOrdersCurrent.load(); }

private:
  void handleMatch(const Order &order, Quantity quantity, Price price) {
    OrderStatus status;
    status.id = order.id;
    status.quantity = quantity;
    status.fillPrice = price;
    status.state = order.quantity == 0 ? OrderState::Full : OrderState::Partial;
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
  std::map<uint32_t, std::vector<Order>, std::greater<uint32_t>> mBids;
  std::map<uint32_t, std::vector<Order>> mAsks;

  Ticker mTicker;
  MatchHandler mHandler;

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> mOrdersCurrent;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP