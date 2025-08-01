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
  explicit Storage(Bus &bus, PostgresAdapter &dbAdapter)
      : bus_{bus}, dbAdapter_{dbAdapter}, marketData_{loadMarketData()},
        persist_{ServerConfig::cfg.orderBookPersist} {}

  void save() {
    if (!persist_) {
      return;
    }
    using namespace utils;
    LOG_INFO_SYSTEM("Saving orders");

    size_t ordersSaved{0};
    Timestamp lastLog{getTimestamp()};
    for (const auto &data : marketData_) {
      const auto &bids = data.second->orderBook.bids();
      const auto &asks = data.second->orderBook.asks();

      TableWriter<ServerOrder> bidsWriter{bids};
      TableWriter<ServerOrder> asksWriter{asks};
      if (!dbAdapter_.write(bidsWriter) || !dbAdapter_.write(asksWriter)) {
        LOG_ERROR_SYSTEM("Failed to persist orders");
        return;
      }
      ordersSaved += bids.size() + asks.size();
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
      if (marketData_.count(order.order.ticker) == 0) {
        LOG_ERROR_SYSTEM("Invalid ticker loaded {}", toString(order.order.ticker));
        continue;
      }
      const auto &data = marketData_.at(order.order.ticker);
      data->orderBook.add(order);
    }
    dbAdapter_.clean(DbTypeMapper<ServerOrder>::table());
  }

  auto marketData() const -> CRef<MarketData> { return marketData_; }

private:
  auto loadMarketData() -> MarketData {
    LOG_DEBUG("Loading market data");

    const auto result = dbAdapter_.readTickers();
    if (!result) {
      LOG_ERROR("Failed to load ticker data");
      throw std::runtime_error(utils::toString(result.error()));
    }
    const auto &prices = result.value();
    MarketData data;
    data.reserve(prices.size());
    const auto workers = ServerConfig::cfg.coresApp.size();

    size_t idx{0};
    const size_t tickerPerWorker{prices.size() / workers};
    for (const auto &item : prices) {
      LOG_TRACE("{}: ${}", utils::toString(item.ticker), item.price);
      const size_t workerId = std::min(idx / tickerPerWorker, workers - 1);
      data.emplace(item.ticker, std::make_unique<TickerData>(bus_, workerId, item.price));
      ++idx;
    }
    LOG_INFO("Data loaded for {} tickers", data.size());
    return data;
  }

private:
  Bus &bus_;

  PostgresAdapter &dbAdapter_;
  const MarketData marketData_;

  const bool persist_;
};

} // namespace hft::server

#endif // HFT_SERVER_STORAGE_HPP