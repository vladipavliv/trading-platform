/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP
#define HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP

#include <pqxx/pqxx>

#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Postgres adapter
 * @todo For better configurability and because adapters won't participate in hot paths,
 * its better to use interfaces here and make adapter factory.
 * @todo Currently adapters operate over the SystemBus, which is single-threaded, so
 * thread-safety of adapters is not a concern.
 */
class PostgresAdapter {
  /**
   * @todo Move to config file
   */
  static constexpr auto CONNECTION_STRING =
      "dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432 connect_timeout=1";
  static constexpr auto SELECT_TICKERS_QUERY = "SELECT * FROM tickers";
  static constexpr auto TICKERS_COUNT_QUERY = "SELECT COUNT(*) FROM tickers";
  static constexpr auto SET_TIMEOUT_QUERY = "SET statement_timeout = 1000";

public:
  PostgresAdapter() : conn_{CONNECTION_STRING} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
  }

  auto readTickers() -> Expected<std::vector<TickerPrice>> {
    try {
      pqxx::work transaction(conn_);
      transaction.exec(SET_TIMEOUT_QUERY);
      const pqxx::result countResult = transaction.exec(TICKERS_COUNT_QUERY);

      std::vector<TickerPrice> tickers;
      if (countResult.empty()) {
        LOG_WARN("Empty tickers table");
        return tickers;
      }
      const size_t count = countResult[0][0].as<size_t>();
      if (count == 0) {
        LOG_WARN("Empty tickers table");
        return tickers;
      }

      tickers.reserve(count);
      const pqxx::result tickersResult = transaction.exec(SELECT_TICKERS_QUERY);

      for (auto row : tickersResult) {
        const String ticker = row["ticker"].as<String>();
        Price price = row["price"].as<size_t>();
        tickers.emplace_back(TickerPrice{utils::toTicker(ticker), price});
      }
      transaction.commit();
      return tickers;
    } catch (const std::exception &e) {
      LOG_ERROR("Exception during tickers read {}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

  auto checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId> {
    LOG_DEBUG("Authenticating {} {}", name, password);
    try {
      pqxx::work transaction(conn_);
      // Set small timeout, systemBus must be responsive.
      // Maybe later on make a separate DataBus for such operations
      transaction.exec("SET statement_timeout = 50");

      const String query = "SELECT client_id, password FROM clients WHERE name = $1";
      const auto result = transaction.exec_params(query, name);

      if (!result.empty()) {
        const ClientId clientId = result[0][0].as<ClientId>();
        const String realPassword = result[0][1].as<String>(); // TODO(self) encrypt
        if (password == realPassword) {
          LOG_INFO("Authentication successfull");
          return clientId;
        } else {
          return std::unexpected(StatusCode::AuthInvalidPassword);
        }
      } else {
        return std::unexpected(StatusCode::AuthUserNotFound);
      }
    } catch (const pqxx::sql_error &e) {
      LOG_ERROR("{}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

private:
  pqxx::connection conn_;
};

} // namespace hft

#endif // HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP