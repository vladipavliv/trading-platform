/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP
#define HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <pqxx/pqxx>
#include <pqxx/stream_to>

#include "config/config.hpp"
#include "logging.hpp"
#include "table_reader.hpp"
#include "table_writer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::adapters {

/**
 * @brief PostgresAdapter
 * @details Not thread-safe, used only in SystemBus
 */
class PostgresAdapter {
  static constexpr auto SELECT_TICKERS_QUERY = "SELECT * FROM tickers";
  static constexpr auto TICKERS_COUNT_QUERY = "SELECT COUNT(*) FROM tickers";
  static constexpr auto SELECT_CLIENT_QUERY =
      "SELECT client_id, password FROM clients WHERE name = $1";
  static constexpr size_t RUNTIME_TIMEOUT_MS = 50;

public:
  PostgresAdapter() : connectionString_{getConnectionString()}, conn_{connectionString_} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
  }

  auto readTickers(bool cache = true) -> Expected<Span<const TickerPrice>> {
    try {
      static std::vector<TickerPrice> tickers;
      if (!cache) {
        tickers.clear();
      } else if (!tickers.empty()) {
        return Span<const TickerPrice>{tickers};
      }
      pqxx::work transaction(conn_);
      transaction.exec_params("SET statement_timeout = $1", RUNTIME_TIMEOUT_MS);
      const pqxx::result countResult = transaction.exec(TICKERS_COUNT_QUERY);

      if (countResult.empty() || countResult[0][0].as<size_t>() == 0) {
        LOG_WARN("Empty tickers table");
        return Span<const TickerPrice>{tickers};
      }
      const size_t count = countResult[0][0].as<size_t>();

      tickers.reserve(count);
      const pqxx::result tickersResult = transaction.exec(SELECT_TICKERS_QUERY);

      for (auto row : tickersResult) {
        const String ticker = row["ticker"].as<String>();
        const Price price = row["price"].as<size_t>();
        tickers.emplace_back(TickerPrice{utils::toTicker(ticker), price});
      }
      transaction.commit();
      return Span<const TickerPrice>{tickers};
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception during tickers read {}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

  auto checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId> {
    LOG_DEBUG("Sensitive information, please look away. Authenticating {} {}", name, password);
    try {
      pqxx::work transaction(conn_);
      // Set small timeout, systemBus must be responsive.
      transaction.exec_params("SET statement_timeout = $1", RUNTIME_TIMEOUT_MS);
      const auto result = transaction.exec_params(SELECT_CLIENT_QUERY, name);

      if (!result.empty()) {
        const ClientId clientId = result[0][0].as<ClientId>();
        const String realPassword = result[0][1].as<String>();
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

  auto getWriter(StringView table) -> Expected<TableWriter> {
    try {
      return std::expected<TableWriter, StatusCode>{std::in_place, conn_, table};
    } catch (CRef<std::exception> e) {
      LOG_ERROR("{}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

  auto getReader(StringView table) -> Expected<TableReader> {
    try {
      return std::expected<TableReader, StatusCode>{std::in_place, conn_, table};
    } catch (CRef<std::exception> e) {
      LOG_ERROR("{}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

  void clean(CRef<String> table) {
    try {
      pqxx::work transaction(conn_);
      const auto query = "DELETE FROM " + table + ";";
      transaction.exec(query);
      transaction.commit();
    } catch (CRef<pqxx::sql_error> e) {
      LOG_ERROR_SYSTEM("pqxx::sql_error {}", e.what());
    } catch (CRef<std::exception> e) {
      LOG_ERROR_SYSTEM("std::exception {}", e.what());
    }
  }

private:
  /**
   * @brief cicd compatibility
   */
  String getConnectionString() const {
    using namespace utils;
    const String host = getEnvVar("POSTGRES_HOST");
    const String port = getEnvVar("POSTGRES_PORT");
    const String user = getEnvVar("POSTGRES_USER");
    const String password = getEnvVar("POSTGRES_PASSWORD");
    const String dbName = getEnvVar("POSTGRES_DB");

    return std::format("host={} port={} user={} password={} dbname={} connect_timeout=1",
                       host.empty() ? Config::get<String>("db.host") : host,
                       port.empty() ? Config::get<String>("db.port") : port,
                       user.empty() ? Config::get<String>("db.user") : user,
                       password.empty() ? Config::get<String>("db.password") : password,
                       dbName.empty() ? Config::get<String>("db.dbname") : dbName);
  }

private:
  const String connectionString_;
  pqxx::connection conn_;
};

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP

#pragma GCC diagnostic pop