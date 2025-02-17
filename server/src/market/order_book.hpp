/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_ORDER_BOOK_HPP
#define HFT_SERVER_ORDER_BOOK_HPP

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "market_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"

namespace hft::server {

/**
 * @brief For simulation sake if order not matched immediately TraderId gets reset
 * to simulate lots of traders and not spam client side
 */
class OrderBook {
public:
  using MatchHandler = std::function<void(const OrderStatus &)>;
  using UPtr = std::unique_ptr<OrderBook>;

  OrderBook() = delete;
  OrderBook(Ticker ticker, MatchHandler matcher) : mTicker{ticker}, mHandler{std::move(matcher)} {
    assert(mHandler != nullptr);
  }

  void add(const Order &order) {
    // TODO(self) Batch add
    if (order.action == OrderAction::Buy) {
      if (mBids.size() > ORDER_BOOK_LIMIT) {
        spdlog::error("Order limit reached for {}", utils::toString(mTicker));
        return;
      }
      auto &priceBids = mBids[order.price];
      if (priceBids.empty()) { // Was just created
        priceBids.reserve(ORDER_BOOK_LIMIT);
      }
      priceBids.push_back(order);
    } else {
      if (mAsks.size() > ORDER_BOOK_LIMIT) {
        spdlog::error("Order limit reached for {}", utils::toString(mTicker));
        return;
      }
      auto &priceBids = mAsks[order.price];
      if (priceBids.empty()) { // Was just created
        priceBids.reserve(ORDER_BOOK_LIMIT);
      }
      priceBids.push_back(order);
    }
    match(order.id);
    /* Randomizer: change Traderid in this order */
    if (mBids.contains(order.price) && !mBids[order.price].empty()) {
      // It could be a different trader but thats fine
      mBids[order.price].back().traderId = utils::RNG::rng(77777);
    }
    if (mAsks.contains(order.price) && !mAsks[order.price].empty()) {
      mAsks[order.price].back().traderId = utils::RNG::rng(77777);
    }
  }

  void match(OrderId id) {
    // TODO() Buffer matches for the same TraderId to send them in bulk
    while (!mBids.empty() && !mAsks.empty()) {
      auto &bestBids = mBids.begin()->second;
      auto &bestAsks = mAsks.begin()->second;

      if (bestBids.front().price < bestAsks.front().price) {
        break;
      }
      auto &bestBid = bestBids.front();
      auto &bestAsk = bestAsks.front();

      int quantity = std::min(bestBid.quantity, bestAsk.quantity);

      uint8_t bidState =
          (bestBid.quantity == quantity) ? (uint8_t)OrderState::Full : (uint8_t)OrderState::Partial;
      uint8_t askState =
          (bestAsk.quantity == quantity) ? (uint8_t)OrderState::Full : (uint8_t)OrderState::Partial;

      if (bestBid.id == id) {
        bidState |= static_cast<uint8_t>(OrderState::Instant);
      }
      if (bestAsk.id == id) {
        askState |= static_cast<uint8_t>(OrderState::Instant);
      }

      bestBid.quantity -= quantity;
      bestAsk.quantity -= quantity;

      handleMatch(bestBid, quantity, bidState, bestAsk.price);
      handleMatch(bestAsk, quantity, askState, bestAsk.price);

      if (bestBid.quantity == 0) {
        bestBids.erase(bestBids.begin());
        if (bestBids.empty()) {
          mBids.erase(mBids.begin());
        }
      }
      if (bestAsk.quantity == 0) {
        bestAsks.erase(bestAsks.begin());
        if (bestAsks.empty()) {
          mAsks.erase(mAsks.begin());
        }
      }
    }
  }

  /**
   * @details CAUTION! Not thread-safe. Call on your own risk.
   * Its just for testing
   */
  inline size_t ordersCount() const { return mBids.size() + mAsks.size(); }

private:
  void handleMatch(const Order &order, Quantity quantity, uint8_t state, Price price) {
    OrderStatus status;
    status.id = order.id;
    status.quantity = quantity;
    status.fillPrice = price;
    status.state = (OrderState)state;
    status.traderId = order.traderId;
    status.ticker = order.ticker;
    mHandler(status);
  }

private:
  std::map<double, std::vector<Order>, std::greater<double>> mBids;
  std::map<double, std::vector<Order>> mAsks;

  Ticker mTicker;
  MatchHandler mHandler;
};

} // namespace hft::server

#endif // HFT_SERVER_ORDER_BOOK_HPP