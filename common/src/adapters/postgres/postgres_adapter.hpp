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
 * @details Interface-type communication feels more natural here as opposed to bus-type
 * @todo Make interfaces and adapter factory, these adapters wont participate in the hot paths,
 * so extra configurability and testability is more prefferable here
 * @todo At the moment adapters operate over SystemBus, which is single-threaded, so no need
 * to worry about thread safety in adapters. Later on if separate DataBus is made to keep
 * SystemBus responsive, thread safety of adapters should be reconsidered.
 * This adapter probably could use some mutex, it handles initial data read and creds verification,
 * which are not the hottest paths. But maybe postgres adapter could work over SystemBus anyway
 * as it shouldn't have overwhelming traffic. Think it through.
 */
class PostgresAdapter {
  /**
   * @todo Move to config file
   */
  static constexpr auto CONNECTION_STRING =
      "dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432 connect_timeout=1";

public:
  PostgresAdapter() : conn_{CONNECTION_STRING} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
  }

  auto readTickers() -> Expected<std::vector<TickerPrice>> {
    try {
      pqxx::work transaction(conn_);
      transaction.exec("SET statement_timeout = 1000");

      const String countQuery = "SELECT COUNT(*) FROM tickers";
      const pqxx::result countResult = transaction.exec(countQuery);

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

      const String query = "SELECT * FROM tickers";
      const pqxx::result res = transaction.exec(query);

      for (auto row : res) {
        std::string ticker = row["ticker"].as<std::string>();
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