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

#include "logging.hpp"

#include "bus/busable.hpp"
#include "config/server_config.hpp"
#include "constants.hpp"
#include "container_types.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "server_domain_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::server {

/**
 * @brief Flat order book
 * @details Since testing is done locally, all the orders have the same client id
 * so every match would come with two notifications, about recent order and a previous one
 * To avoid this the order status is sent only about the most recent order
 */
class OrderBook {
  struct InternalOrder {
    ClientId clientId;
    OrderId id;
    Timestamp created;
    Quantity quantity;
    Price price;

    inline void partialFill(Quantity amount) {
      quantity = quantity < amount ? 0 : quantity - amount;
    }
  };

  static inline bool compareBids(CRef<InternalOrder> left, CRef<InternalOrder> right) {
    return left.price < right.price;
  }
  static inline bool compareAsks(CRef<InternalOrder> left, CRef<InternalOrder> right) {
    return left.price > right.price;
  }
  static inline ServerOrderStatus getStatus( // format
      CRef<InternalOrder> o, Quantity quantity, Price price, OrderState state) {
    return ServerOrderStatus(o.clientId, {o.id, 0, quantity, price, state});
  }
  static inline ServerOrderStatus getStatus( // format
      CRef<ServerOrder> o, Quantity quantity, Price price, OrderState state) {
    return ServerOrderStatus(o.clientId, {o.order.id, 0, quantity, price, state});
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

  bool add(CRef<ServerOrder> order, BusableFor<ServerOrderStatus> auto &consumer) {
    if (bids_.size() + asks_.size() >= ServerConfig::cfg.orderBookLimit) {
      LOG_DEBUG_SYSTEM("OrderBook limit reached: {}", openedOrders_.load());
      consumer.post(getStatus(order, 0, 0, OrderState::Rejected));
      return false;
    }
    if (order.order.action == OrderAction::Buy) {
      bids_.push_back(InternalOrder{order.clientId, order.order.id, order.order.created,
                                    order.order.quantity, order.order.price});
      std::push_heap(bids_.begin(), bids_.end(), compareBids);
    } else {
      asks_.push_back(InternalOrder{order.clientId, order.order.id, order.order.created,
                                    order.order.quantity, order.order.price});
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);
    }
    return true;
  }
#if defined(BENCHMARK_BUILD) || defined(UNIT_TESTS_BUILD)
  void sendAck(CRef<ServerOrder> order, BusableFor<ServerOrderStatus> auto &consumer) {
    consumer.post(ServerOrderStatus{order.clientId, order.order.id, 0, 0, 0, OrderState::Accepted});
  }

  void clear() {
    bids_.clear();
    asks_.clear();
    openedOrders_ = 0;
  }
#endif

  void match(BusableFor<ServerOrderStatus> auto &consumer) {
    while (!bids_.empty() && !asks_.empty()) {
      InternalOrder &bestBid = bids_.front();
      InternalOrder &bestAsk = asks_.front();
      if (bestBid.price < bestAsk.price) {
        break;
      }
      const auto quantity = std::min(bestBid.quantity, bestAsk.quantity);
      bestBid.partialFill(quantity);
      bestAsk.partialFill(quantity);

      // local workaround, if client id is the same, send only a recent order notification
      if ((bestBid.clientId != bestAsk.clientId) || (bestBid.created > bestAsk.created)) {
        const auto state = (bestBid.quantity == 0) ? OrderState::Full : OrderState::Partial;
        consumer.post(getStatus(bestBid, quantity, bestAsk.price, state));
      }
      if ((bestBid.clientId != bestAsk.clientId) || (bestAsk.created > bestBid.created)) {
        const auto state = (bestAsk.quantity == 0) ? OrderState::Full : OrderState::Partial;
        consumer.post(getStatus(bestAsk, quantity, bestAsk.price, state));
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
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
  }

  auto extract() const -> Vector<ServerOrder> {
    Vector<ServerOrder> orders;
    //    orders.reserve(bids_.size() + asks_.size());
    //    orders.insert(orders.end(), bids_.begin(), bids_.end());
    //    orders.insert(orders.end(), asks_.begin(), asks_.end());
    return orders;
  }

  auto extract() -> Vector<ServerOrder> {
    Vector<ServerOrder> orders;
    // orders.reserve(bids_.size() + asks_.size());
    // orders.insert(orders.end(), std::make_move_iterator(bids_.begin()),
    //               std::make_move_iterator(bids_.end()));
    // orders.insert(orders.end(), std::make_move_iterator(asks_.begin()),
    //               std::make_move_iterator(asks_.end()));
    // bids_.clear();
    // asks_.clear();
    // openedOrders_ = 0;
    return orders;
  }

  void inject(Span<const ServerOrder> orders) {
    /*for (const auto &order : orders) {
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
    }*/
  }

  inline size_t openedOrders() const { return openedOrders_.load(std::memory_order_relaxed); }

private:
  OrderBook(const OrderBook &) = delete;
  OrderBook &operator=(const OrderBook &other) = delete;

private:
  alignas(64) Vector<InternalOrder> bids_;
  alignas(64) Vector<InternalOrder> asks_;

  alignas(64) std::atomic_size_t openedOrders_{0};
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
