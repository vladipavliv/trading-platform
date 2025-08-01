/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_STORAGE_HPP
#define HFT_SERVER_STORAGE_HPP

#include "adapters/postgres/postgres_adapter.hpp"
#include "db_table_reader.hpp"
#include "db_table_writer.hpp"
#include "db_type_mapper.hpp"
#include "execution/server_ticker_data.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft::server {

class Storage {
public:
  explicit Storage(PostgresAdapter &dbAdapter, CRef<MarketData> marketData)
      : dbAdapter_{dbAdapter}, marketData_{marketData} {}

  void saveOrders() {
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
    LOG_INFO_SYSTEM("Opened orders have been saved successfully");
  }

  void loadOrders() {
    LOG_INFO_SYSTEM("Loading orders");

    TableReader<ServerOrder> reader{};
    if (!dbAdapter_.read(reader)) {
      LOG_ERROR_SYSTEM("Failed to load orders");
      return;
    }
    const auto &orders = reader.result();
    if (orders.empty()) {
      return;
    }
    LOG_INFO_SYSTEM("Orders loaded: {}", utils::thousandify(orders.size()));

    for (const auto &order : orders) {
      if (marketData_.count(order.order.ticker) == 0) {
        LOG_ERROR_SYSTEM("Invalid ticker loaded {}", utils::toString(order.order.ticker));
        continue;
      }
      const auto &data = marketData_.at(order.order.ticker);
      data->orderBook.add(order);
    }
    dbAdapter_.clean(DbTypeMapper<ServerOrder>::table());
  }

private:
  PostgresAdapter &dbAdapter_;
  CRef<MarketData> marketData_;
};

} // namespace hft::server

#endif // HFT_SERVER_STORAGE_HPP