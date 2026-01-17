/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_SERVER_PRICELEVELORDERBOOK_HPP
#define HFT_SERVER_PRICELEVELORDERBOOK_HPP

#include "bus/busable.hpp"
#include "gateway/internal_order.hpp"
#include "gateway/internal_order_status.hpp"
#include "id/slot_id.hpp"
#include "id/slot_id_pool.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "schema.hpp"
#include "utils/huge_array.hpp"

namespace hft::server {

/**
 * @brief
 */
class PriceLevelOrderBook {
  enum class Side : uint8_t { Buy, Sell };

  struct Node {
    BookOrderId localId;
    SystemOrderId systemId;

    uint32_t next;
    uint32_t prev;

    uint32_t price;
    uint32_t qty;

    Side side;
  };

  struct PriceLevel {
    uint32_t head;
    uint32_t tail;
    uint64_t volume;
  };

  struct BestPrice {
    Price price;
    bool exists;
  };

  static constexpr uint32_t MASK_SIZE = (MAX_TICKS / 64) + 1;

public:
  PriceLevelOrderBook() : freeTop_{0}, nextAvailableIdx_{1} {
    std::memset(bidMask_, 0, sizeof(bidMask_));
    std::memset(askMask_, 0, sizeof(askMask_));
  }

  bool add(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    if (ioe.action == OrderAction::Cancel) {
      cancelOrder(ioe, consumer);
      return true;
    }

    auto &o = ioe.order;
    const Side side = getSide(ioe.action);
    uint32_t remainingQty = match(o, side, consumer);

    if (remainingQty > 0) {
      restOrder(o, side, remainingQty, consumer);
    } else {
      consumer.post(
          InternalOrderStatus{o.id, BookOrderId{}, o.quantity, o.price, OrderState::Full});
    }

    return true;
  }

#if defined(BENCHMARK_BUILD) || defined(UNIT_TESTS_BUILD)
  void sendAck(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    consumer.post(InternalOrderStatus(ioe.order.id, BookOrderId{}, 0, 0, OrderState::Accepted));
  }

  void clear() {
    std::memset(bidMask_, 0, sizeof(bidMask_));
    std::memset(askMask_, 0, sizeof(askMask_));

    freeTop_ = 0;
    nextAvailableIdx_ = 1;

    bids_.clear();
    asks_.clear();
  }
#endif

private:
  uint32_t match(CRef<InternalOrder> o, Side side, BusableFor<InternalOrderStatus> auto &consumer) {
    uint32_t remainingQty = o.quantity;

    while (remainingQty > 0) {
      auto bestPrice = (side == Side::Buy) ? getBestAsk() : getBestBid();

      if (!bestPrice.exists)
        break;
      if (side == Side::Buy && bestPrice.price > o.price)
        break;
      if (side == Side::Sell && bestPrice.price < o.price)
        break;

      PriceLevel &level = (side == Side::Buy) ? asks_[bestPrice.price] : bids_[bestPrice.price];

      while (level.head != 0 && remainingQty > 0) {
        Node &node = nodePool_[level.head];
        uint32_t fillQty = std::min(remainingQty, node.qty);
        remainingQty -= fillQty;

        const bool isFullFill = (fillQty == node.qty);
        const auto status = isFullFill ? OrderState::Full : OrderState::Partial;

        node.qty -= fillQty;
        level.volume -= fillQty;

        if (isFullFill) {
          uint32_t oldHead = level.head;
          level.head = node.next;

          if (level.head != 0) {
            nodePool_[level.head].prev = 0;
          } else {
            level.tail = 0;
            updateOccupancy(node.side, bestPrice.price, false);
          }
          releaseId(node.localId);
        }
      }
    }
    return remainingQty;
  }

  void restOrder(CRef<InternalOrder> o, Side side, uint32_t qty,
                 BusableFor<InternalOrderStatus> auto &consumer) {
    BookOrderId localId = acquireId();
    if (UNLIKELY(!localId)) {
      LOG_ERROR("OrderBook is full");
      return;
    }

    uint32_t idx = localId.index();
    Node &node = nodePool_[idx];
    node.localId = localId;
    node.systemId = o.id;
    node.price = o.price;
    node.qty = qty;
    node.side = side;
    node.next = 0;

    PriceLevel &level = (side == Side::Buy) ? bids_[node.price] : asks_[node.price];
    node.prev = level.tail;

    if (level.tail != 0) {
      nodePool_[level.tail].next = idx;
    } else {
      level.head = idx;
      updateOccupancy(side, node.price, true);
    }
    level.tail = idx;
    level.volume += node.qty;

    const uint32_t totalFilled = o.quantity - qty;
    const auto state = (totalFilled == 0) ? OrderState::Accepted : OrderState::Partial;

    consumer.post(InternalOrderStatus{o.id, localId, totalFilled, o.price, state});
  }

  void cancelOrder(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    auto &o = ioe.order;
    uint32_t idx = o.bookOId.index();
    Node &node = nodePool_[idx];

    if (UNLIKELY(node.localId != o.bookOId)) {
      LOG_ERROR("Invalid book oid");
      return;
    }

    PriceLevel &level = (node.side == Side::Buy) ? bids_[o.price] : asks_[o.price];

    if (node.prev != 0) {
      nodePool_[node.prev].next = node.next;
    } else {
      level.head = node.next;
    }
    if (node.next != 0) {
      nodePool_[node.next].prev = node.prev;
    } else {
      level.tail = node.prev;
    }

    level.volume -= node.qty;

    if (level.volume == 0) {
      updateOccupancy(node.side, o.price, false);
    }

    consumer.post(InternalOrderStatus{o.id, o.bookOId, 0, 0, OrderState::Cancelled});
    releaseId(node.localId);
  }

  inline void updateOccupancy(Side side, uint32_t priceIdx, bool active) {
    uint64_t *mask = (side == Side::Buy) ? bidMask_ : askMask_;

    const uint32_t wordIdx = priceIdx >> 6;
    const uint64_t bit = 1ULL << (priceIdx & 63);

    if (active) {
      mask[wordIdx] |= bit;
    } else {
      mask[wordIdx] &= ~bit;
    }
  }

  BestPrice getBestBid() const {
    for (int i = static_cast<int>(MASK_SIZE) - 1; i >= 0; --i) {
      if (bidMask_[i] != 0) {
        Price p = (i << 6) + (63 - __builtin_clzll(bidMask_[i]));
        return {p, true};
      }
    }
    return {0, false};
  }

  BestPrice getBestAsk() const {
    for (uint32_t i = 0; i < MASK_SIZE; ++i) {
      if (askMask_[i] != 0) {
        Price p = (i << 6) + __builtin_ctzll(askMask_[i]);
        return {p, true};
      }
    }
    return {0, false};
  }

  inline auto acquireId() -> BookOrderId {
    if (LIKELY(freeTop_ > 0)) {
      return freeStack_[--freeTop_];
    }
    if (UNLIKELY(nextAvailableIdx_ >= MAX_BOOK_ORDERS)) {
      return BookOrderId{};
    }
    return BookOrderId::make(nextAvailableIdx_++, 1);
  }

  inline void releaseId(BookOrderId idx) {
    idx.nextGen();
    freeStack_[freeTop_++] = idx;
  }

  inline Side getSide(OrderAction action) const {
    return action == OrderAction::Buy ? Side::Buy : Side::Sell;
  }

  inline Side invert(Side side) const { return side == Side::Buy ? Side::Sell : Side::Buy; }

private:
  HugeArray<Node, MAX_BOOK_ORDERS> nodePool_;
  HugeArray<PriceLevel, MAX_TICKS> bids_;
  HugeArray<PriceLevel, MAX_TICKS> asks_;

  ALIGN_CL uint64_t bidMask_[MASK_SIZE];
  ALIGN_CL uint64_t askMask_[MASK_SIZE];

  BookOrderId freeStack_[MAX_BOOK_ORDERS];
  uint32_t freeTop_;
  uint32_t nextAvailableIdx_;
};
} // namespace hft::server

#endif // HFT_SERVER_PRICELEVELORDERBOOK_HPP