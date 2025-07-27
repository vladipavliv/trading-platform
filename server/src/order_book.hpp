/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_ORDERBOOK_HPP
#define HFT_SERVER_ORDERBOOK_HPP

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "constants.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief Flat order book
 * @details Since testing is done locally, all the orders have the same client id
 * so every match would come with two notifications, about recent order and a previous one
 * To avoid this the last added order ids are saved to a set and only those get notifications
 * @todo At some point it might make sense to split Order to hot and cold data
 * Currently for simple limit orders all the data is hot, and there are no iterations
 * without sending status, so separating it doesnt make sense for now.
 * Reconsider for more complex order types later on.
 */
class OrderBook {
  static inline bool compareBids(CRef<ServerOrder> left, CRef<ServerOrder> right) {
    return left.order.price < right.order.price;
  }
  static inline bool compareAsks(CRef<ServerOrder> left, CRef<ServerOrder> right) {
    return left.order.price > right.order.price;
  }

public:
  OrderBook() {
    bids_.reserve(ORDER_BOOK_LIMIT);
    asks_.reserve(ORDER_BOOK_LIMIT);
  }
  ~OrderBook() = default;

  void add(CRef<ServerOrder> order) {
    if (order.order.action == OrderAction::Buy) {
      bids_.push_back(order);
      std::push_heap(bids_.begin(), bids_.end(), compareBids);
    } else {
      asks_.push_back(order);
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);
    }
    lastAdded_ = order.order.id;
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
  }

  template <typename Consumer>
  void match(Consumer &&consumer) {
    while (!bids_.empty() && !asks_.empty()) {
      ServerOrder &bestBid = bids_.front();
      ServerOrder &bestAsk = asks_.front();
      if (bestBid.order.price < bestAsk.order.price) {
        break;
      }
      const auto quantity = std::min(bestBid.order.quantity, bestAsk.order.quantity);
      bestBid.order.partialFill(quantity);
      bestAsk.order.partialFill(quantity);

      if (lastAdded_ == bestBid.order.id) {
        consumer(getMatch(bestBid, quantity, bestAsk.order.price));
      }
      if (lastAdded_ == bestAsk.order.id) {
        consumer(getMatch(bestAsk, quantity, bestAsk.order.price));
      }

      if (bestBid.order.quantity == 0) {
        std::pop_heap(bids_.begin(), bids_.end(), compareBids);
        bids_.pop_back();
      }
      if (bestAsk.order.quantity == 0) {
        std::pop_heap(asks_.begin(), asks_.end(), compareAsks);
        asks_.pop_back();
      }
    }
    lastAdded_ = 0;
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
  }

  inline size_t openedOrders() const { return openedOrders_.load(std::memory_order_relaxed); }

private:
  ServerOrderStatus getMatch(CRef<ServerOrder> o, Quantity quantity, Price price) {
    return ServerOrderStatus(
        o.clientId, OrderStatus{o.order.id, utils::getTimestamp(), quantity, price,
                                (o.order.quantity == 0) ? OrderState::Full : OrderState::Partial});
  }

private:
  std::vector<ServerOrder> bids_;
  std::vector<ServerOrder> asks_;
  OrderId lastAdded_;
  std::atomic_uint64_t openedOrders_;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
