/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_STORAGE_HPP
#define HFT_SERVER_STORAGE_HPP

#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/bus.hpp"
#include "db_table_reader.hpp"
#include "db_table_writer.hpp"
#include "db_type_mapper.hpp"
#include "execution/server_ticker_data.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft::server {

class Storage {
public:
  explicit Storage(PostgresAdapter &dbAdapter)
      : dbAdapter_{dbAdapter}, marketData_{loadMarketData()},
        persist_{ServerConfig::cfg.orderBookPersist} {}

  void save() {
    if (!persist_) {
      return;
    }
    using namespace utils;
    LOG_INFO_SYSTEM("Saving orders");

    size_t ordersSaved{0};
    Timestamp lastLog{getTimestamp()};
    const size_t workerCount = ServerConfig::cfg.coresApp.size();

    for (const auto &tkrData : marketData_) {
      const auto orders = tkrData.second.orderBook.extract();
      TableWriter<ServerOrder> ordersWriter{orders};
      if (!dbAdapter_.write(ordersWriter)) {
        LOG_ERROR_SYSTEM("Failed to persist orders");
        return;
      }
      ordersSaved += orders.size();
      const auto now = utils::getTimestamp();
      if (now - lastLog > 1000000) {
        LOG_INFO_SYSTEM("Saved {} orders", thousandify(ordersSaved));
        lastLog = now;
      }
    }
    LOG_INFO_SYSTEM("Saved {} orders", thousandify(ordersSaved));
    LOG_INFO_SYSTEM("Opened orders have been saved successfully");
  }

  void load() {
    if (!persist_) {
      return;
    }
    LOG_INFO_SYSTEM("Loading orders");
    using namespace utils;

    TableReader<ServerOrder> reader{};
    if (!dbAdapter_.read(reader)) {
      LOG_ERROR_SYSTEM("Failed to load orders");
      return;
    }
    const auto &orders = reader.result();
    if (orders.empty()) {
      return;
    }
    LOG_INFO_SYSTEM("Orders loaded: {}", thousandify(orders.size()));

    for (const auto &order : orders) {
      auto it = marketData_.find(order.order.ticker);
      if (it == marketData_.end()) {
        LOG_ERROR_SYSTEM("Invalid ticker loaded {}", toString(order.order.ticker));
        continue;
      }
      it->second.orderBook.inject({&order, 1});
    }

    dbAdapter_.clean(DbTypeMapper<ServerOrder>::table());
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
    const ThreadId workerCount = ServerConfig::cfg.coresApp.size();
    MarketData data;
    data.reserve(prices.size());

    const size_t perWorker = prices.size() / workerCount;
    const size_t leftOver = prices.size() % workerCount;

    auto iter = prices.begin();
    for (ThreadId idx = 0; idx < workerCount; ++idx) {
      const size_t currWorkerTickers = perWorker + (idx < leftOver ? 1 : 0);
      for (size_t i = 0; i < currWorkerTickers && iter != prices.end(); ++i, ++iter) {
        LOG_TRACE("{}: ${}", toString(it->ticker), it->price);
        data.emplace(iter->ticker, TickerData{idx});
      }
    }
    LOG_INFO("Data loaded for {} tickers", prices.size());
    return data;
  }

private:
  PostgresAdapter &dbAdapter_;
  const MarketData marketData_;
  const bool persist_;
};

} // namespace hft::server

#endif // HFT_SERVER_STORAGE_HPP