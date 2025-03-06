/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_ORDERBOOK_HPP
#define HFT_SERVER_ORDERBOOK_HPP

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.hpp"
#include "market_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief Flat order book
 * @details since testing is done via single trader, all the orders have the same trader id
 * so every match would come with two notifications, about recent order and a previous one
 * To avoid this the last added order ids are saved to a set and only for those notifications
 * about match are sent
 */
class OrderBook {
  static bool compareBids(const Order &left, const Order &right) {
    return left.price < right.price;
  }
  static bool compareAsks(const Order &left, const Order &right) {
    return left.price > right.price;
  }

public:
  using UPtr = std::unique_ptr<OrderBook>;

  OrderBook() {
    mBids.reserve(ORDER_BOOK_LIMIT);
    mAsks.reserve(ORDER_BOOK_LIMIT);
  }
  ~OrderBook() = default;

  void add(const Order &order) {
    if (order.action == OrderAction::Buy) {
      mBids.push_back(order);
      std::push_heap(mBids.begin(), mBids.end(), compareBids);
    } else {
      mAsks.push_back(order);
      std::push_heap(mAsks.begin(), mAsks.end(), compareAsks);
    }
    mLastAdded.insert(order.id); // Randomizator
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
    spdlog::trace([&status] { return utils::toString(status); }());
    return status;
  }

private:
  std::vector<Order> mBids;
  std::vector<Order> mAsks;
  std::set<OrderId> mLastAdded;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
