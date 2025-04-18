/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_DB_POSTGRESADAPTER_HPP
#define HFT_COMMON_DB_POSTGRESADAPTER_HPP

#include <pqxx/pqxx>

#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Postgres adapter
 * @details Handles auth requests over the system bus
 */
class PostgresAdapter {
  static constexpr auto CONNECTION_STRING =
      "dbname=hft_db user=postgres password=password host=127.0.0.1 port=5432 connect_timeout=1";

public:
  PostgresAdapter() : conn_{CONNECTION_STRING} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
  }

  std::vector<TickerPrice> readTickers() {
    try {
      pqxx::work transaction(conn_);
      transaction.exec("SET statement_timeout = 1000");

      const String countQuery = "SELECT COUNT(*) FROM tickers";
      const pqxx::result countResult = transaction.exec(countQuery);

      if (countResult.empty()) {
        LOG_ERROR("Empty tickers table");
        return {};
      }
      const size_t count = countResult[0][0].as<size_t>();
      if (count == 0) {
        LOG_ERROR("Empty tickers table");
        return {};
      }

      std::vector<TickerPrice> tickers;
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
      return {};
    }
  }

  Expected<ClientId> checkCredentials(CRef<String> name, CRef<String> password) {
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
          return std::unexpected("Invalid password");
        }
      } else {
        return std::unexpected("User not found");
      }
    } catch (const pqxx::sql_error &e) {
      return std::unexpected(String(e.what()));
    }
  }

private:
  pqxx::connection conn_;
};

} // namespace hft

#endif // HFT_COMMON_DB_POSTGRESADAPTER_HPP