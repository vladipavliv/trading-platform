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

#include "concepts/busable.hpp"
#include "config/server_config.hpp"
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
 * @todo Works with the single ticker, and has separate container for bids and asks
 * So ticker and action could be thrown away to reduce memory footprint
 * Would complicate things a bit, but help keeping order under 32 bytes and have 2 orders
 * in a single cache line
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

  template <Busable Consumer>
  bool add(CRef<ServerOrder> order, Consumer &consumer) {
    if (openedOrders_ >= ServerConfig::cfg.orderBookLimit) {
      LOG_DEBUG_SYSTEM("OrderBook limit reached: {}", openedOrders_);
      consumer.post(getStatus(order, 0, 0, OrderState::Rejected));
      return false;
    }
    if (order.order.action == OrderAction::Buy) {
      bids_.push_back(order);
      std::push_heap(bids_.begin(), bids_.end(), compareBids);
    } else {
      asks_.push_back(order);
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);
    }
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
    return true;
  }

  template <Busable Consumer>
  void match(Consumer &consumer) {
#ifdef BENCHMARK_BUILD
    bool notified{false};
#endif
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
        consumer.post(getStatus(bestBid, quantity, bestAsk.order.price, state));
#ifdef BENCHMARK_BUILD
        notified = true;
#endif
      } else if ((bestBid.clientId != bestAsk.clientId) ||
                 (bestAsk.order.created > bestBid.order.created)) {
        const auto state = (bestAsk.order.quantity == 0) ? OrderState::Full : OrderState::Partial;
        consumer.post(getStatus(bestAsk, quantity, bestAsk.order.price, state));
#ifdef BENCHMARK_BUILD
        notified = true;
#endif
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
#ifdef BENCHMARK_BUILD
    if (!notified) {
      consumer.post(ServerOrderStatus{0, 0, 0, 0, 0, OrderState::Accepted});
    }
#endif
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
    for (const auto &order : orders) {
      if (openedOrders() >= ServerConfig::cfg.orderBookLimit) {
        LOG_ERROR_SYSTEM("Failed to inject orders: limit reached");
        break;
      }
      if (order.order.action == OrderAction::Buy) {
        bids_.push_back(order);
        std::push_heap(bids_.begin(), bids_.end(), compareBids);

      } else {
        asks_.push_back(order);
        std::push_heap(asks_.begin(), asks_.end(), compareAsks);
      }
    }
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
