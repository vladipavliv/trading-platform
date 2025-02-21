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
#include "template_types.hpp"
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
  using MatchHandler = SpanHandler<OrderStatus>;
  using UPtr = std::unique_ptr<MapOrderBook>;

  MapOrderBook() = delete;
  MapOrderBook(Ticker ticker, MatchHandler matcher)
      : mTicker{ticker}, mHandler{std::move(matcher)} {
    assert(mHandler != nullptr);
    mMatchesBuffer.reserve(20);
  }

  void add(Span<Order> orders) {
    mOrdersCurrent.fetch_add(1);
    bool matchingNeeded{false};
    // TODO() Remove later, this is optimization for the current test setup
    // Only the best order gets notification
    TraderId matchingId;
    for (auto &order : orders) {
      if (order.action == OrderAction::Buy) {
        if (mBids.size() > ORDER_BOOK_LIMIT) {
          spdlog::error("Order limit reached for {}", utils::toStrView(mTicker));
          continue;
        }
        // If bid is lower then current best bid - we can skip matching
        if (mBids.empty() || order.price > mBids.begin()->first) {
          matchingNeeded = true;
          matchingId = order.id;
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
        if (mAsks.empty() || order.price < mAsks.begin()->first) {
          matchingNeeded = true;
          matchingId = order.id;
        }
        auto &priceBids = mAsks[order.price];
        if (priceBids.empty()) {
          priceBids.reserve(ORDER_BOOK_LIMIT);
        }
        priceBids.push_back(order);
      }
    }
    if (matchingNeeded) {
      match(matchingId);
    }
  }

  /**
   * @brief Simulate tracking active users with passing order id
   */
  void match(OrderId activeOrder) {
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

      onOrderMatched(bestBid, quantity, bestAsk.price, activeOrder);
      onOrderMatched(bestAsk, quantity, bestAsk.price, activeOrder);

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
    mHandler(Span<OrderStatus>{mMatchesBuffer});
    mMatchesBuffer.clear();
  }

  inline size_t ordersCount() const { return mOrdersCurrent.load(); }

private:
  void onOrderMatched(const Order &order, Quantity quantity, Price price, OrderId activeOrder) {
    if (order.id != activeOrder) {
      return;
    }
    OrderStatus status;
    status.id = order.id;
    status.quantity = quantity;
    status.fillPrice = price;
    status.state = order.quantity == 0 ? OrderState::Full : OrderState::Partial;
    status.action = order.action;
    status.traderId = order.traderId;
    status.ticker = order.ticker;
    spdlog::info(utils::toString(status));
    mMatchesBuffer.emplace_back(status);
    if (order.quantity == 0) {
      mOrdersCurrent.fetch_sub(1);
    }
  }

private:
  std::map<uint32_t, std::vector<Order>, std::greater<uint32_t>> mBids;
  std::map<uint32_t, std::vector<Order>> mAsks;

  Ticker mTicker;
  MatchHandler mHandler;
  std::vector<OrderStatus> mMatchesBuffer;

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> mOrdersCurrent;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
