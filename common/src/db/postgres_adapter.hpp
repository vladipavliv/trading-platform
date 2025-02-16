/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DB_POSTGRESADAPTER_HPP
#define HFT_COMMON_DB_POSTGRESADAPTER_HPP

#include <ctime>
#include <iostream>
#include <pqxx/pqxx>
#include <random>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

#include "market_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::db {

class PostgresAdapter {
public:
  static std::vector<PriceUpdate> readTickers() {
    pqxx::connection conn("dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432");

    if (!conn.is_open()) {
      std::cerr << "Failed to connect to the database" << std::endl;
      return;
    }
    std::vector<PriceUpdate> tickers;
    pqxx::work txn(conn);

    std::string query = "SELECT * FROM tickers";
    pqxx::result res = txn.exec(query);

    for (auto row : res) {
      std::string ticker = row["ticker"].as<std::string>();
      Price price = row["price"].as<double>();
      tickers.push_back(PriceUpdate{utils::toTicker(ticker), price});
    }
    txn.commit();
    return tickers;
  }

  static void generateABunchOfTickers(uint16_t size) {
    pqxx::connection pgConn(
        "dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432");
    if (!pgConn.is_open()) {
      spdlog::error("Failed to connect to postgres");
      assert(false);
      return;
    }
    pqxx::work pgWork(pgConn);

    std::stringstream query;
    query << "INSERT INTO tickers (ticker, price) VALUES ";

    for (int i = 0; i < size; ++i) {
      PriceUpdate ticker = utils::generatePriceUpdate();
      std::string tickerStr(ticker.ticker.begin(), ticker.ticker.end());

      pgWork.exec_params("INSERT INTO tickers (ticker, price) VALUES ($1, $2)", tickerStr,
                         ticker.price);
    }
    pgWork.commit();
  }
};

} // namespace hft::db

#endif // HFT_COMMON_DB_POSTGRESADAPTER_HPP