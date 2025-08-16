/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_STORAGE_HPP
#define HFT_SERVER_STORAGE_HPP

#include "adapters/adapters.hpp"
#include "execution/server_market_data.hpp"
#include "logging.hpp"
#include "server_type_converters.hpp"
#include "type_converters.hpp"
#include "types.hpp"

namespace hft::server {

class Storage {
public:
  explicit Storage(adapters::DbAdapter &dbAdapter)
      : dbAdapter_{dbAdapter}, marketData_{loadMarketData()},
        persist_{ServerConfig::cfg.orderBookPersist} {}

  void save() {
    if (!persist_) {
      return;
    }
    using namespace utils;

    auto writerExp = dbAdapter_.getWriter("orders");
    if (!writerExp) {
      LOG_ERROR("Failed to save data");
      return;
    }
    LOG_INFO_SYSTEM("Saving orders");

    auto &writer = writerExp.value();

    size_t ordersSaved{0};
    Timestamp lastLog{getTimestamp()};

    for (const auto &tkrData : marketData_) {
      const auto orders = tkrData.second.orderBook.extract();

      for (auto &order : orders) {
        writer << order;
      }

      ordersSaved += orders.size();
      const auto now = utils::getTimestamp();
      if (Microseconds(now - lastLog) > ServerConfig::cfg.monitorRate) {
        LOG_INFO_SYSTEM("Saved {} orders", ordersSaved);
        lastLog = now;
      }
    }
    writer.commit();

    LOG_INFO_SYSTEM("Saved {} orders", ordersSaved);
    LOG_INFO_SYSTEM("Opened orders have been saved successfully");
  }

  void load() {
    if (!persist_) {
      return;
    }
    LOG_INFO_SYSTEM("Loading orders");
    using namespace utils;

    auto readerExp = dbAdapter_.getReader("orders");
    if (!readerExp) {
      LOG_ERROR("Failed to load data");
      return;
    }
    auto &reader = readerExp.value();
    Vector<ServerOrder> orders;
    orders.reserve(reader.size());

    size_t ordersSaved{0};
    Timestamp lastLog{getTimestamp()};

    while (reader.next()) {
      ServerOrder order;
      reader >> order;
      orders.push_back(order);

      const auto now = utils::getTimestamp();
      if (Microseconds(now - lastLog) > ServerConfig::cfg.monitorRate) {
        LOG_INFO_SYSTEM("Loaded {} orders", orders.size());
        lastLog = now;
      }
    }
    reader.commit();

    LOG_INFO_SYSTEM("Orders loaded: {}", orders.size());

    for (const auto &order : orders) {
      auto it = marketData_.find(order.order.ticker);
      if (it == marketData_.end()) {
        LOG_ERROR_SYSTEM("Invalid ticker loaded {}", toString(order.order.ticker));
        continue;
      }
      it->second.orderBook.inject({&order, 1});
    }

    dbAdapter_.clean("orders");
  }

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
        data.emplace(iter->ticker, TickerData{idx});
      }
    }
    LOG_INFO("Data loaded for {} tickers", prices.size());
    return data;
  }

private:
  adapters::DbAdapter &dbAdapter_;

  const MarketData marketData_;
  const bool persist_;
};

} // namespace hft::server

#endif // HFT_SERVER_STORAGE_HPP