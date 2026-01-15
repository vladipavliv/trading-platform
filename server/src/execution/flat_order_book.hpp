/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_FLATORDERBOOK_HPP
#define HFT_SERVER_FLATORDERBOOK_HPP

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "logging.hpp"

#include "bus/busable.hpp"
#include "config/server_config.hpp"
#include "constants.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "gateway/internal_order.hpp"
#include "gateway/internal_order_status.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/huge_array.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::server {

/**
 * @brief Flat order book
 * @details Since testing is done locally, all the orders have the same client id
 * so every match would come with two notifications, about recent order and a previous one
 * To avoid this the order status is sent only about the most recent order
 */
class FlatOrderBook {
  static inline bool compareBids(CRef<InternalOrder> left, CRef<InternalOrder> right) {
    return left.price < right.price;
  }
  static inline bool compareAsks(CRef<InternalOrder> left, CRef<InternalOrder> right) {
    return left.price > right.price;
  }

  static constexpr size_t MAX_ORDERS = 131072;

public:
  FlatOrderBook() {}

  FlatOrderBook(FlatOrderBook &&other) noexcept
      : bids_(std::move(other.bids_)), asks_(std::move(other.asks_)) {}

  FlatOrderBook &operator=(FlatOrderBook &&other) noexcept {
    if (this != &other) {
      bids_ = std::move(other.bids_);
      asks_ = std::move(other.asks_);
    }
    return *this;
  }

  bool add(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    auto &side = (ioe.action == OrderAction::Buy) ? bids_ : asks_;
    auto &count = (ioe.action == OrderAction::Buy) ? bidCount_ : askCount_;

    if (UNLIKELY(count >= MAX_ORDERS)) {
      LOG_ERROR_SYSTEM("OrderBook side limit reached!");
      consumer.post(InternalOrderStatus(ioe.order.id, 0, 0, OrderState::Rejected));
      return false;
    }

    side[count] = ioe.order;
    count++;

    if (ioe.action == OrderAction::Buy) {
      std::push_heap(&side[0], &side[bidCount_], compareBids);
    } else {
      std::push_heap(&side[0], &side[askCount_], compareAsks);
    }

    lastAdded_ = ioe.order.id;
    return true;
  }

  void match(BusableFor<InternalOrderStatus> auto &consumer) {
    while (bidCount_ > 0 && askCount_ > 0) {
      auto &bestBid = bids_[0];
      auto &bestAsk = asks_[0];

      if (bestBid.price < bestAsk.price)
        break;

      const auto matchQty = std::min(bestBid.quantity, bestAsk.quantity);
      const auto matchPrice = bestAsk.price;

      bestBid.partialFill(matchQty);
      bestAsk.partialFill(matchQty);

      if (lastAdded_ == bestBid.id) {
        consumer.post(
            InternalOrderStatus(bestBid.id, matchQty, matchPrice,
                                (bestBid.quantity == 0) ? OrderState::Full : OrderState::Partial));
      }

      if (lastAdded_ == bestAsk.id) {
        consumer.post(
            InternalOrderStatus(bestAsk.id, matchQty, matchPrice,
                                (bestAsk.quantity == 0) ? OrderState::Full : OrderState::Partial));
      }

      if (bestBid.quantity == 0) {
        std::pop_heap(&bids_[0], &bids_[bidCount_], compareBids);
        bidCount_--;
      }
      if (bestAsk.quantity == 0) {
        std::pop_heap(&asks_[0], &asks_[askCount_], compareAsks);
        askCount_--;
      }
    }
  }

#if defined(BENCHMARK_BUILD) || defined(UNIT_TESTS_BUILD)
  void sendAck(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    consumer.post(InternalOrderStatus(ioe.order.id, 0, 0, OrderState::Accepted));
  }

  void clear() {
    bidCount_ = 0;
    askCount_ = 0;
    lastAdded_ = InternalOrderId();
  }
#endif

private:
  FlatOrderBook(const FlatOrderBook &) = delete;
  FlatOrderBook &operator=(const FlatOrderBook &other) = delete;

private:
  ALIGN_CL HugeArray<InternalOrder, MAX_ORDERS> bids_;
  ALIGN_CL HugeArray<InternalOrder, MAX_ORDERS> asks_;

  uint32_t bidCount_{0};
  uint32_t askCount_{0};
  InternalOrderId lastAdded_;
};

} // namespace hft::server

#endif // HFT_SERVER_FLATORDERBOOK_HPP
