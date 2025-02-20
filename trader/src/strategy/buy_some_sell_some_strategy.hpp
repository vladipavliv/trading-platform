/**
 * @author Vladimir Pavliv
 * @date 2025-02-14
 */

#ifndef HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP
#define HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP

#include <atomic>
#include <deque>

#include "boost_types.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "trader_types.hpp"
#include "trading_stats.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/string_utils.hpp"

namespace hft::trader {

/**
 * @brief AKA BSSS Strategy
 */
class BuySomeSellSomeStrategy {
public:
  BuySomeSellSomeStrategy(TraderSink &sink)
      : mSink{sink}, mPrices{db::PostgresAdapter::readTickers()},
        mBalance{utils::RNG::rng<double>(1000000)}, mTradeRate{Config::cfg.tradeRateUs},
        mTimer{mSink.ctx()} {
    mSink.dataSink.setHandler<TickerPrice>(
        [this](const TickerPrice &price) { priceUpdate(price); });
    mSink.dataSink.setHandler<OrderStatus>(
        [this](const OrderStatus &status) { orderStatus(status); });
    mSink.controlSink.addCommandHandler(
        {TraderCommand::TradeStart, TraderCommand::TradeStop, TraderCommand::TradeSwitch,
         TraderCommand::CollectStats, TraderCommand::TradeSpeedUp, TraderCommand::TradeSpeedDown},
        [this](TraderCommand cmd) { processCommand(cmd); });
  }

private:
  void priceUpdate(const TickerPrice &newPrice) { spdlog::debug(utils::toString(newPrice)); }
  void orderStatus(const OrderStatus &status) {
    size_t rtt = static_cast<uint32_t>(utils::getLinuxTimestamp()) - status.id;
    std::string scaleStr = utils::getScaleNs(rtt);
    std::string statusStr = utils::toString(status);
    spdlog::info(std::format("{} {} {}", statusStr, "RTT: ", scaleStr, rtt));

    if (!Config::cfg.monitorStats) {
      return;
    }
    auto stats = mStats.load();
    if (rtt > (stats.rttBest * 100)) {
      stats.rttSpikes++;
    } else {
      stats.operations++;
      stats.rttSum += rtt;
      stats.rttBest = rtt < stats.rttBest ? rtt : stats.rttBest;
    }

    stats.rttWorst = rtt > stats.rttWorst ? rtt : stats.rttWorst;
    auto amount = status.fillPrice * status.quantity;
    (status.action == OrderAction::Buy) ? (stats.balance += amount) : (stats.balance -= amount);
    mStats.store(stats);
  }

  void tradeSwitch(bool start) {
    mTrading.store(start);
    if (mTrading) {
      tradeCycle();
    } else {
      mTimer.cancel();
    }
  }

  void tradeSomething() {
    auto tickerPrice = mPrices[utils::RNG::rng(mPrices.size() - 1)];
    Order order;
    order.id = utils::getLinuxTimestamp();
    order.ticker = tickerPrice.ticker;
    order.price = utils::RNG::rng<uint32_t>(tickerPrice.price * 2);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(1000);
    spdlog::debug("Placing order {}", utils::toString(order));
    mSink.ioSink.post(order);
  }

  void collectStats() {
    auto stats = mStats.load();
    mSink.controlSink.onEvent(stats);
  }

  void processCommand(TraderCommand cmd) {
    switch (cmd) {
    case TraderCommand::TradeStart:
      tradeSwitch(true);
      break;
    case TraderCommand::TradeStop:
      tradeSwitch(false);
      break;
    case TraderCommand::TradeSwitch:
      tradeSwitch(!mTrading);
      break;
    case TraderCommand::CollectStats:
      collectStats();
      break;
    case TraderCommand::TradeSpeedUp:
      if (mTradeRate > 100) {
        mTradeRate.store(mTradeRate / 2);
      }
      spdlog::info("Trade rate: {}", utils::getScaleUs(mTradeRate));
      break;
    case TraderCommand::TradeSpeedDown:
      mTradeRate.store(mTradeRate * 2);
      spdlog::info("Trade rate: {}", utils::getScaleUs(mTradeRate));
      break;
    default:
      break;
    }
  }

private:
  void tradeCycle() {
    if (!mTrading.load()) {
      return;
    }
    mTimer.expires_after(Microseconds(mTradeRate));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        tradeSomething();
        tradeCycle();
      }
    });
  }

private:
  TraderSink &mSink;
  std::vector<TickerPrice> mPrices;
  std::atomic<TradingStats> mStats;
  double mBalance;

  std::atomic<size_t> mTradeRate;
  std::atomic_bool mTrading;
  SteadyTimer mTimer;
};

} // namespace hft::trader

#endif // HFT_TRADER_BUYSOMESELLSOMESTRATEGY_HPP