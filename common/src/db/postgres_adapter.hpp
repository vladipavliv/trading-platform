/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DB_POSTGRESADAPTER_HPP
#define HFT_COMMON_DB_POSTGRESADAPTER_HPP

#include <sstream>
#include <string>

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

#include "bus/bus.hpp"
#include "market_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Postgres adapter
 * @details Some requests are direct, some go over the system bus
 */
class PostgresAdapter {
  static constexpr auto CONNECTION_STRING =
      "dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432 connect_timeout=1";

public:
  explicit PostgresAdapter(SystemBus &bus) : bus_{bus}, conn_{CONNECTION_STRING} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
    bus_.subscribe<CredentialsLoginRequest>(
        [this](CRef<CredentialsLoginRequest> request) { onAuthenticate(request); });
  }

  std::vector<TickerPrice> readTickers() {
    try {
      pqxx::work transaction(conn_);
      transaction.exec("SET statement_timeout = 1000");

      std::string countQuery = "SELECT COUNT(*) FROM tickers";
      pqxx::result countResult = transaction.exec(countQuery);

      if (countResult.empty()) {
        LOG_ERROR("Empty tickers table");
        return {};
      }
      size_t count = countResult[0][0].as<size_t>();
      if (count == 0) {
        LOG_ERROR("Empty tickers table");
        return {};
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
    } catch (const std::exception &e) {
      LOG_ERROR("Exception during tickers read {}", e.what());
      return {};
    }
  }

private:
  void onAuthenticate(CRef<CredentialsLoginRequest> request) {
    try {
      pqxx::work transaction(conn_);
      transaction.exec("SET statement_timeout = 50");

      std::string query = "SELECT trader_id, password FROM traders WHERE name = $1";
      pqxx::result result = transaction.exec_params(query, request.name);

      bool authSuccessfull{false};
      if (!result.empty()) {
        TraderId traderId = result[0][0].as<TraderId>();
        std::string password = result[0][1].as<std::string>(); // TODO(self) encrypt
        if (request.password == password) {
          authSuccessfull = true;
          bus_.post(LoginResponse{request.socketId, traderId, 0, true});
        }
      }
      if (!authSuccessfull) {
        bus_.post(LoginResponse{request.socketId, 0, 0, false});
      }
    } catch (const pqxx::sql_error &e) {
      LOG_ERROR("onAuthenticate exception {}", e.what());
      bus_.post(LoginResponse{request.socketId, 0, 0, false});
    }
  }

private:
  SystemBus &bus_;
  pqxx::connection conn_;
};

} // namespace hft

#endif // HFT_COMMON_DB_POSTGRESADAPTER_HPP