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
  explicit OrderBook(Bus &bus) : bus_{bus}, orderBookLimit_{ServerConfig::cfg.orderBookLimit} {
    bids_.reserve(orderBookLimit_);
    asks_.reserve(orderBookLimit_);
  }
  ~OrderBook() = default;

  bool add(CRef<ServerOrder> order) {
    bool hasMatch{true};
    if (openedOrders_ >= orderBookLimit_) {
      LOG_ERROR_SYSTEM("OrderBook limit reached {}", openedOrders_);
      LOG_ERROR_SYSTEM("Rejecting order {}", utils::toString(order))
      bus_.post(getStatus(order, 0, 0, OrderState::Rejected));
      return false;
    }
    if (order.order.action == OrderAction::Buy) {
      bids_.push_back(order);
      std::push_heap(bids_.begin(), bids_.end(), compareBids);

      // Send accepted only if no immediate match, otherwise rtt drops quite a bit
      if (!asks_.empty()) {
        ServerOrder &bestAsk = asks_.front();
        if (order.order.price < bestAsk.order.price) {
          bus_.post(getStatus(order, 0, order.order.price, OrderState::Accepted));
          hasMatch = false;
        }
      }
    } else {
      asks_.push_back(order);
      std::push_heap(asks_.begin(), asks_.end(), compareAsks);

      if (!bids_.empty()) {
        ServerOrder &bestBid = bids_.front();
        if (order.order.price > bestBid.order.price) {
          bus_.post(getStatus(order, 0, order.order.price, OrderState::Accepted));
          hasMatch = false;
        }
      }
    }
    openedOrders_.store(bids_.size() + asks_.size(), std::memory_order_relaxed);
    return hasMatch;
  }

  void match() {
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
        bus_.post(getStatus(bestBid, quantity, bestAsk.order.price, state));
      }
      if ((bestBid.clientId != bestAsk.clientId) ||
          (bestAsk.order.created > bestBid.order.created)) {
        const auto state = (bestAsk.order.quantity == 0) ? OrderState::Full : OrderState::Partial;
        bus_.post(getStatus(bestAsk, quantity, bestAsk.order.price, state));
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

  inline size_t openedOrders() const { return openedOrders_.load(std::memory_order_relaxed); }

  auto bids() const -> CRef<std::vector<ServerOrder>> { return bids_; }

  auto asks() const -> CRef<std::vector<ServerOrder>> { return asks_; }

private:
  Bus &bus_;

  std::vector<ServerOrder> bids_;
  std::vector<ServerOrder> asks_;

  std::atomic_size_t openedOrders_;
  const size_t orderBookLimit_;
};

} // namespace hft::server

#endif // HFT_SERVER_MAPORDERBOOK_HPP
