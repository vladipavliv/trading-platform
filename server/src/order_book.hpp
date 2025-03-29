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
 * To avoid this the last added order ids are saved to a set and only those get notifications
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
    bids_.reserve(ORDER_BOOK_LIMIT);
    asks_.reserve(ORDER_BOOK_LIMIT);
  }
  ~OrderBook() = default;

  void add(const Order &order) {
    if (order.action == OrderAction::Buy) {
      bids_.push_back(order);
      std::push_heap(bids_.begin(), bids_.end(), compareBids);
    } else {
      asks_.push_back(order);
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);
    }
    lastAdded_.insert(order.id);
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
  }

  std::vector<OrderStatus> match() {
    std::vector<OrderStatus> matches;
    matches.reserve(10);

    while (!bids_.empty() && !asks_.empty()) {
      Order &bestBid = bids_.front();
      Order &bestAsk = asks_.front();
      if (bestBid.price < bestAsk.price) {
        break;
      }
      auto quantity = std::min(bestBid.quantity, bestAsk.quantity);
      bestBid.quantity -= quantity;
      bestAsk.quantity -= quantity;

      if (lastAdded_.contains(bestBid.id)) {
        matches.emplace_back(handleMatch(bestBid, quantity, bestAsk.price));
      }
      if (lastAdded_.contains(bestAsk.id)) {
        matches.emplace_back(handleMatch(bestAsk, quantity, bestAsk.price));
      }

      if (bestBid.quantity == 0) {
        std::pop_heap(bids_.begin(), bids_.end(), compareBids);
        bids_.pop_back();
      }
      if (bestAsk.quantity == 0) {
        std::pop_heap(asks_.begin(), asks_.end(), compareAsks);
        asks_.pop_back();
      }
    }
    lastAdded_.clear();
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
    return matches;
  }

  inline size_t openedOrders() const { return openedOrders_.load(std::memory_order_relaxed); }

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
    spdlog::debug([&status] { return utils::toString(status); }());
    return status;
  }

private:
  std::vector<Order> bids_;
  std::vector<Order> asks_;
  std::set<OrderId> lastAdded_;
  std::atomic_uint64_t openedOrders_;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
