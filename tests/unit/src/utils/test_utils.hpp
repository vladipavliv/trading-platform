/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_UNIT_TESTUTILS_HPP
#define HFT_UNIT_TESTUTILS_HPP

#include "config/config.hpp"
#include "container_types.hpp"
#include "gateway/internal_id.hpp"
#include "gateway/internal_order.hpp"
#include "gateway/internal_order_status.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "utils/id_utils.hpp"
#include "utils/rng.hpp"
#include "utils/sync_utils.hpp"
#include "utils/test_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::tests {

inline Ticker generateTicker() {
  Ticker result;
  for (size_t i = 0; i < 4; ++i) {
    result[i] = utils::RNG::generate<uint8_t>((uint8_t)'A', (uint8_t)'Z');
  }
  return result;
}

inline Order generateOrder(Ticker ticker = {'G', 'O', 'O', 'G'}) {
  return Order{utils::generateOrderId(),
               utils::getCycles(),
               ticker,
               utils::RNG::generate<Quantity>(0, 1000),
               utils::RNG::generate<Price>(10, 10000),
               utils::RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell};
}

inline server::InternalOrderEvent generateInternalOrder() {
  using namespace server;
  Order o = generateOrder();
  return server::InternalOrderEvent{
      {InternalOrderId{o.id}, o.quantity, o.price}, nullptr, o.ticker, o.action};
}

struct alignas(64) Consumer {
  explicit Consumer(size_t target = 0) : target{target}, processed{0}, signal{0} {}

  const size_t target;
  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic<uint32_t> signal;
  alignas(64) std::atomic<bool> flag;
  alignas(64) std::atomic<bool> shutdown;

  template <typename Message>
  void post(const Message &) {
    auto counter = processed.fetch_add(1, std::memory_order_relaxed);
    if (counter + 1 >= target) {
      signal.store(1, std::memory_order_release);
      utils::futexWake(signal);
    }
  }

  void wait() { utils::hybridWait(signal, 0, flag, shutdown); }
  void clear() { signal.store(0, std::memory_order_relaxed); }
};

} // namespace hft::tests

#endif // HFT_UNIT_TESTUTILS_HPP