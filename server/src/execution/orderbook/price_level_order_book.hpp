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

class PriceLevelOrderBook {
  enum class Side : uint8_t { Buy, Sell };

  struct Node {
    uint32_t next;
    uint32_t prev;

    uint32_t price;
    uint32_t qty;

    BookOrderId localId;
    SystemOrderId systemId;

    Side side;
  };

  struct PriceLevelSide {
    uint64_t volume;
    uint32_t head;
    uint32_t tail;
  };

  struct PriceLevel {
    PriceLevelSide bid;
    PriceLevelSide ask;
  };

  struct BestPrice {
    Price price;
    bool exists;
  };

  static constexpr uint32_t MASK_SIZE = (MAX_TICKS / 64) + 1;

public:
  PriceLevelOrderBook() : freeTop_{0}, nextAvailableIdx_{1}, minAsk_{MAX_TICKS}, maxBid_{0} {
    std::memset(bidMask_, 0, sizeof(bidMask_));
    std::memset(askMask_, 0, sizeof(askMask_));
  }

  bool add(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    LOG_DEBUG("Add order {}", toString(ioe));
    if (ioe.action == OrderAction::Cancel) {
      cancelOrder(ioe, consumer);
      return true;
    }

    auto &o = ioe.order;
    const Side side = getSide(ioe.action);

    if (UNLIKELY(o.price >= MAX_TICKS)) [[unlikely]] {
      LOG_ERROR("Price out of bounds: %u", o.price);
      return false;
    }

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
    minAsk_ = MAX_TICKS;
    maxBid_ = 0;
    levels_.clear();
    nodePool_.clear();
  }
#endif

private:
  uint32_t match(CRef<InternalOrder> o, Side side, BusableFor<InternalOrderStatus> auto &consumer) {
    LOG_DEBUG("Match {}", toString(o));
    uint32_t remainingQty = o.quantity;

    while (remainingQty > 0) {
      if (side == Side::Buy) {
        if (minAsk_ > o.price)
          break;
      } else {
        if (maxBid_ < o.price)
          break;
      }

      auto bestPrice = (side == Side::Buy) ? getBestAsk(o.price) : getBestBid(o.price);
      if (!bestPrice.exists)
        break;

      auto &pricePoint = levels_[bestPrice.price];
      PriceLevelSide &level = (side == Side::Buy) ? pricePoint.ask : pricePoint.bid;

      while (level.head != 0 && remainingQty > 0) {
        Node &restingNode = nodePool_[level.head];
        uint32_t fillQty = std::min(remainingQty, restingNode.qty);

        remainingQty -= fillQty;
        restingNode.qty -= fillQty;
        level.volume -= fillQty;

        if (restingNode.qty == 0) {
          level.head = restingNode.next;
          if (level.head != 0) {
            nodePool_[level.head].prev = 0;
          } else {
            level.tail = 0;
            updateOccupancy(restingNode.side, bestPrice.price, false);
          }
          releaseId(restingNode.localId);
        }
      }
    }
    return remainingQty;
  }

  void restOrder(CRef<InternalOrder> o, Side side, uint32_t qty,
                 BusableFor<InternalOrderStatus> auto &consumer) {
    LOG_DEBUG("restOrder {}", toString(o));
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

    auto &pricePoint = levels_[node.price];
    PriceLevelSide &level = (side == Side::Buy) ? pricePoint.bid : pricePoint.ask;

    node.prev = level.tail;

    if (level.tail != 0) {
      nodePool_[level.tail].next = idx;
    } else {
      level.head = idx;
      updateOccupancy(side, node.price, true);
    }
    level.tail = idx;
    level.volume += node.qty;

    const uint32_t fillTtl = o.quantity - qty;
    const auto state = (fillTtl == 0) ? OrderState::Accepted : OrderState::Partial;

    consumer.post(InternalOrderStatus{o.id, localId, fillTtl, o.price, state});
  }

  void cancelOrder(CRef<InternalOrderEvent> ioe, BusableFor<InternalOrderStatus> auto &consumer) {
    auto &o = ioe.order;
    uint32_t idx = o.bookOId.index();
    Node &node = nodePool_[idx];

    if (UNLIKELY(node.localId != o.bookOId)) {
      LOG_ERROR("Invalid book oid");
      return;
    }

    auto &pricePoint = levels_[node.price];
    PriceLevelSide &level = (node.side == Side::Buy) ? pricePoint.bid : pricePoint.ask;

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
      updateOccupancy(node.side, node.price, false);
    }

    consumer.post(InternalOrderStatus{o.id, o.bookOId, 0, 0, OrderState::Cancelled});
    releaseId(node.localId);
  }

  inline void updateOccupancy(Side side, uint32_t priceIdx, bool active) {
    LOG_DEBUG("updateOccupancy");
    uint64_t *mask = (side == Side::Buy) ? bidMask_ : askMask_;
    const uint32_t wordIdx = priceIdx >> 6;
    const uint64_t bit = 1ULL << (priceIdx & 63);

    if (active) {
      mask[wordIdx] |= bit;
      if (side == Side::Buy)
        maxBid_ = std::max(maxBid_, priceIdx);
      else
        minAsk_ = std::min(minAsk_, priceIdx);
    } else {
      mask[wordIdx] &= ~bit;
      if (side == Side::Buy && priceIdx == maxBid_)
        findNewMaxBid(wordIdx);
      else if (side == Side::Sell && priceIdx == minAsk_)
        findNewMinAsk(wordIdx);
    }
  }

  void findNewMaxBid(uint32_t startWord) {
    for (int i = static_cast<int>(startWord); i >= 0; --i) {
      if (bidMask_[i] != 0) {
        maxBid_ = (i << 6) + (63 - __builtin_clzll(bidMask_[i]));
        return;
      }
    }
    maxBid_ = 0;
  }

  void findNewMinAsk(uint32_t startWord) {
    for (uint32_t i = startWord; i < MASK_SIZE; ++i) {
      if (askMask_[i] != 0) {
        minAsk_ = (i << 6) + __builtin_ctzll(askMask_[i]);
        return;
      }
    }
    minAsk_ = MAX_TICKS;
  }

  BestPrice getBestAsk(Price limitPrice) const {
    const uint32_t startWord = minAsk_ >> 6;
    const uint32_t maxWord = limitPrice >> 6;
    for (uint32_t i = startWord; i <= maxWord; ++i) {
      if (askMask_[i] != 0) {
        Price p = (i << 6) + __builtin_ctzll(askMask_[i]);
        if (p <= limitPrice)
          return {p, true};
        break;
      }
    }
    return {0, false};
  }

  BestPrice getBestBid(Price limitPrice) const {
    const int startWord = static_cast<int>(maxBid_ >> 6);
    const int minWord = static_cast<int>(limitPrice >> 6);
    for (int i = startWord; i >= minWord; --i) {
      if (bidMask_[i] != 0) {
        Price p = (i << 6) + (63 - __builtin_clzll(bidMask_[i]));
        if (p >= limitPrice)
          return {p, true};
        break;
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

private:
  HugeArray<Node, MAX_BOOK_ORDERS> nodePool_;
  HugeArray<PriceLevel, MAX_TICKS> levels_;

  ALIGN_CL uint64_t bidMask_[MASK_SIZE];
  ALIGN_CL uint64_t askMask_[MASK_SIZE];

  BookOrderId freeStack_[MAX_BOOK_ORDERS];
  uint32_t freeTop_;
  uint32_t nextAvailableIdx_;

  Price minAsk_;
  Price maxBid_;
};
} // namespace hft::server

#endif
