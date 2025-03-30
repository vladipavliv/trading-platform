/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DB_POSTGRESADAPTER_HPP
#define HFT_COMMON_DB_POSTGRESADAPTER_HPP

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

#include "market_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::db {

/**
 * @brief Postgres adapter
 */
class PostgresAdapter {
  static auto connect() {
    pqxx::connection conn("dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432");
    if (!conn.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
    return conn;
  }

public:
  static std::optional<TraderId> authenticate(CRef<LoginRequest> cred) {
    pqxx::connection conn = connect();
    pqxx::work transaction(conn);

    std::string query = "SELECT trader_id, password FROM traders WHERE name = $1";
    pqxx::result result = transaction.exec_params(query, cred.name);

    if (result.empty()) {
      return false;
    }
    TraderId traderId = result[0][0].as<TraderId>();
    std::string password = result[0][1].as<std::string>(); // TODO(self) encrypt

    return cred.password == password ? traderId : std::optional<TraderId>();
  }

  static std::vector<TickerPrice> readTickers() {
    pqxx::connection conn = connect();
    pqxx::work transaction(conn);

    std::string countQuery = "SELECT COUNT(*) FROM tickers";
    pqxx::result countResult = transaction.exec(countQuery);

    if (countResult.empty()) {
      throw std::runtime_error("Empty tickers table");
    }
    size_t count = countResult[0][0].as<size_t>();
    if (count == 0) {
      throw std::runtime_error("Empty tickers table");
    }

    std::vector<TickerPrice> tickers;
    tickers.reserve(count);

    std::string query = "SELECT * FROM tickers";
    pqxx::result res = transaction.exec(query);

    for (auto row : res) {
      std::string ticker = row["ticker"].as<std::string>();
      Price price = row["price"].as<size_t>();
      tickers.emplace_back(TickerPrice{utils::toTicker(ticker), price});
    }
    transaction.commit();
    return tickers;
  }
};

} // namespace hft::db

#endif // HFT_COMMON_DB_POSTGRESADAPTER_HPP