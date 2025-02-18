/**
 * @author Vladimir Pavliv
 * @date 2025-02-14
 */

#ifndef HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP
#define HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP

#include "boost_types.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "trader_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/string_utils.hpp"

namespace hft::trader {

/**
 * @brief AKA BSSSStrategy
 */
class BuySomeSellSomeStrategy {
public:
  BuySomeSellSomeStrategy(TraderSink &sink)
      : mSink{sink}, mPrices{db::PostgresAdapter::readTickers()}, mTimer{mSink.ctx()} {
    mSink.dataSink.setHandler<TickerPrice>(
        [this](const TickerPrice &price) { priceUpdate(price); });
    mSink.dataSink.setHandler<OrderStatus>(
        [this](const OrderStatus &status) { orderStatus(status); });
    mSink.controlSink.setHandler([this](TraderCommand cmd) {
      if (cmd == TraderCommand::StartTrading) {
        tradeStart();
      } else if (cmd == TraderCommand::StopTrading) {
        tradeStop();
      }
    });
  }

  void start() {}
  void stop() { tradeStop(); }

private:
  void priceUpdate(const TickerPrice &newPrice) { spdlog::debug(utils::toString(newPrice)); }
  void orderStatus(const OrderStatus &status) {
    std::string scaleStr = utils::getScale(utils::getLinuxTimestamp() - status.id);
    std::string statusStr = utils::toString(status);
    spdlog::debug("{} RTT: {}", statusStr, scaleStr);
  }

  void tradeStart() {
    mSpeed += 10;
    mTrading.store(true);
    tradeCycle();
  }
  void tradeStop() {
    mSpeed -= 10;
    mTrading.store(false);
    mTimer.cancel();
  }

  void tradeSomething() {
    auto tickerPrice = mPrices[utils::RNG::rng(mPrices.size() - 1)];
    Order order;
    order.id = utils::getLinuxTimestamp();
    order.ticker = tickerPrice.ticker;
    order.price = utils::RNG::rng<float>(tickerPrice.price * 2);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(1000);
    // spdlog::debug("Placing order {}", utils::toString(order));
    if (utils::empty(order.ticker)) {
      spdlog::error("Ticker is empty {}", utils::toString(order));
      return;
    }
    mSink.networkSink.post(order);
  }

private:
  void tradeCycle() {
    if (!mTrading.load()) {
      return;
    }
    // auto next = MilliSeconds(TRADE_RATE) - MilliSeconds(mSpeed);
    // next = (next < MilliSeconds(1)) ? MilliSeconds(1) : next;
    mTimer.expires_after(boost::asio::chrono::microseconds(100));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        tradeSomething();
        tradeCycle();
      }
    });
  }

  TraderSink &mSink;
  std::vector<TickerPrice> mPrices;

  std::atomic_bool mTrading;
  int mSpeed{0};
  SteadyTimer mTimer;
};

} // namespace hft::trader

#endif // HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP