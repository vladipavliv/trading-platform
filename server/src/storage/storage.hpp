/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_STORAGE_HPP
#define HFT_SERVER_STORAGE_HPP

#include "container_types.hpp"
#include "execution/market_data.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "traits.hpp"

namespace hft::server {

class Storage {
public:
  explicit Storage(DbAdapter &dbAdapter) : dbAdapter_{dbAdapter}, marketData_{loadMarketData()} {}

  auto marketData() const -> CRef<MarketData> { return marketData_; }

private:
  auto loadMarketData() -> MarketData {
    using namespace utils;
    LOG_DEBUG("Loading market data");

    const auto result = dbAdapter_.readTickers();
    if (!result) {
      LOG_ERROR("Failed to load ticker data");
      throw std::runtime_error(toString(result.error()));
    }

    const auto &prices = result.value();
    const ThreadId workerCount =
        ServerConfig::cfg.coresApp.size() == 0 ? 1 : ServerConfig::cfg.coresApp.size();

    MarketData data;
    data.reserve(prices.size());

    const size_t perWorker = prices.size() / workerCount;
    const size_t leftOver = prices.size() % workerCount;

    auto iter = prices.begin();
    for (ThreadId idx = 0; idx < workerCount; ++idx) {
      const size_t currWorkerTickers = perWorker + (idx < leftOver ? 1 : 0);
      for (size_t i = 0; i < currWorkerTickers && iter != prices.end(); ++i, ++iter) {
        LOG_TRACE("{}: ${}", toString(iter->ticker), iter->price);
        data.emplace(iter->ticker, std::make_unique<TickerData>(idx));
      }
    }
    LOG_INFO("Data loaded for {} tickers", prices.size());
    return data;
  }

private:
  DbAdapter &dbAdapter_;

  const MarketData marketData_;
};

} // namespace hft::server

#endif // HFT_SERVER_STORAGE_HPP