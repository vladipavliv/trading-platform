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

#include "bus/bus.hpp"
#include "constants.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

/**
 * @brief Flat order book
 * @details Since testing is done locally, all the orders have the same client id
 * so every match would come with two notifications, about recent order and a previous one
 * To avoid this the order status is sent only about the most recent order
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
  static inline ServerOrderStatus getStatus( // format
      CRef<ServerOrder> o, Quantity quantity, Price price, OrderState state) {
    return ServerOrderStatus(
        o.clientId, OrderStatus{o.order.id, utils::getTimestamp(), quantity, price, state});
  }

public:
  /**
   * @note Not sure about passing bus here, but its convenient. Need to send status
   * not only for fulfillment, but also right away after receiving order.
   */
  OrderBook() {
    bids_.reserve(ServerConfig::cfg.orderBookLimit);
    asks_.reserve(ServerConfig::cfg.orderBookLimit);
  }

  OrderBook(OrderBook &&other) noexcept
      : bids_{std::move(other.bids_)}, asks_{std::move(other.asks_)},
        openedOrders_{other.openedOrders_.load(std::memory_order_acquire)} {};

  OrderBook &operator=(OrderBook &&other) noexcept {
    bids_ = std::move(other.bids_);
    asks_ = std::move(other.asks_);
    openedOrders_ = other.openedOrders_.load(std::memory_order_acquire);
    return *this;
  };

  template <typename Matcher>
  bool add(CRef<ServerOrder> order, Matcher &matcher) {
    bool hasMatch{false};

    if (openedOrders_ >= ServerConfig::cfg.orderBookLimit) {
      // TODO() cleanup
      bids_.clear();
      asks_.clear();
      openedOrders_ = 0;
      return false;

      LOG_ERROR_SYSTEM("OrderBook limit reached: {}", openedOrders_);
      matcher(getStatus(order, 0, 0, OrderState::Rejected));
      return false;
    }
    if (order.order.action == OrderAction::Buy) {
      bids_.push_back(order);
      std::push_heap(bids_.begin(), bids_.end(), compareBids);

      if (!asks_.empty() && order.order.price >= asks_.front().order.price) {
        hasMatch = true;
      }
    } else {
      asks_.push_back(order);
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);

      if (!bids_.empty() && order.order.price <= bids_.front().order.price) {
        hasMatch = true;
      }
    }
    if (hasMatch) {
      matcher(getStatus(order, 0, order.order.price, OrderState::Accepted));
    }
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
    return true;
  }

  template <typename Matcher>
  void match(Matcher &matcher) {
    while (!bids_.empty() && !asks_.empty()) {
      ServerOrder &bestBid = bids_.front();
      ServerOrder &bestAsk = asks_.front();
      if (bestBid.order.price < bestAsk.order.price) {
        break;
      }
      const auto quantity = std::min(bestBid.order.quantity, bestAsk.order.quantity);
      bestBid.order.partialFill(quantity);
      bestAsk.order.partialFill(quantity);

      if ((bestBid.clientId != bestAsk.clientId) ||
          (bestBid.order.created > bestAsk.order.created)) {
        const auto state = (bestBid.order.quantity == 0) ? OrderState::Full : OrderState::Partial;
        matcher(getStatus(bestBid, quantity, bestAsk.order.price, state));
      }
      if ((bestBid.clientId != bestAsk.clientId) ||
          (bestAsk.order.created > bestBid.order.created)) {
        const auto state = (bestAsk.order.quantity == 0) ? OrderState::Full : OrderState::Partial;
        matcher(getStatus(bestAsk, quantity, bestAsk.order.price, state));
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
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
  }

  auto extract() const -> Vector<ServerOrder> {
    Vector<ServerOrder> orders;
    orders.reserve(bids_.size() + asks_.size());
    orders.insert(orders.end(), bids_.begin(), bids_.end());
    orders.insert(orders.end(), asks_.begin(), asks_.end());
    return orders;
  }

  auto extract() -> Vector<ServerOrder> {
    Vector<ServerOrder> orders;
    orders.reserve(bids_.size() + asks_.size());
    orders.insert(orders.end(), std::make_move_iterator(bids_.begin()),
                  std::make_move_iterator(bids_.end()));
    orders.insert(orders.end(), std::make_move_iterator(asks_.begin()),
                  std::make_move_iterator(asks_.end()));
    bids_.clear();
    asks_.clear();
    openedOrders_ = 0;
    return orders;
  }

  void inject(Span<const ServerOrder> orders) {
    size_t injected{0};
    for (const auto &order : orders) {
      if (injected >= ServerConfig::cfg.orderBookLimit) {
        LOG_ERROR_SYSTEM("OrderBook limit reached: {}", injected);
        break;
      }
      if (order.order.action == OrderAction::Buy) {
        bids_.push_back(order);
        std::push_heap(bids_.begin(), bids_.end(), compareBids);

      } else {
        asks_.push_back(order);
        std::push_heap(asks_.begin(), asks_.end(), compareAsks);
      }
      ++injected;
    }
    openedOrders_ = injected;
  }

  inline size_t openedOrders() const { return openedOrders_.load(std::memory_order_relaxed); }

private:
  OrderBook(const OrderBook &) = delete;
  OrderBook &operator=(const OrderBook &other) = delete;

private:
  Vector<ServerOrder> bids_;
  Vector<ServerOrder> asks_;

  std::atomic_size_t openedOrders_{0};
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
