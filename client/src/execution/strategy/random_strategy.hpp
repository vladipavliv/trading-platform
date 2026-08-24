/**
 * @author Vladimir Pavliv
 * @date 2026-08-23
 */

#ifndef HFT_COMMON_RANDOMSTRATEGY_HPP
#define HFT_COMMON_RANDOMSTRATEGY_HPP

#include "execution/market_data.hpp"
#include "execution/order_registry.hpp"
#include "ticker/ticker_map.hpp"
#include "traits.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"

namespace hft::client {
/**
 * @brief Generates random order
 */
class RandomStrategy {
public:
  RandomStrategy(Context &ctx, OrderRegistry &registry, const MarketData &marketData)
      : ctx_{ctx}, registry_{registry}, marketData_{marketData} {}

  void execute() {
    using namespace utils;
    if (tickerCursor_ == TICKER_COUNT) {
      tickerCursor_ = 0;
      if (currentSide_ == OrderAction::Buy) {
        currentSide_ = OrderAction::Sell;
      } else {
        currentSide_ = OrderAction::Buy;
      }
    }
    auto &p = marketData_[tickerCursor_];

    const auto newPrice = fluctuateThePrice(p.getPrice());
    const auto action = currentSide_;
    const auto quantity = RNG::generate<Quantity>(1, 100);

    const auto now = getCycles();
    Order order{0, getTicker(tickerCursor_), quantity, newPrice, action};
    if (!registry_.allocate(order, now)) {
      LOG_DEBUG("Failed to generate new order");
      ctx_.bus.post(InternalError{StatusCode::Error, "Failed to generate new order"});
      return;
    }

    LOG_DEBUG("Placing order {}", toString(order));
    ctx_.bus.marketBus.post(order);
    ++tickerCursor_;
  }

private:
  Context &ctx_;

  OrderRegistry &registry_;
  const MarketData &marketData_;

  uint32_t tickerCursor_{0};
  OrderAction currentSide_{OrderAction::Buy};
};
} // namespace hft::client

#endif // HFT_COMMON_RANDOMSTRATEGY_HPP
