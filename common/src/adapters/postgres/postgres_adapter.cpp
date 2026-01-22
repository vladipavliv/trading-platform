/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#include <format>
#include <pqxx/pqxx>

#include "config/config.hpp"
#include "container_types.hpp"
#include "domain_types.hpp"
#include "functional_types.hpp"
#include "logging.hpp"
#include "postgres_adapter.hpp"
#include "ptr_types.hpp"
#include "utils/parse_utils.hpp"
#include "utils/string_utils.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace hft::adapters {

PostgresAdapter::PostgresAdapter(const Config &cfg)
    : config_{cfg}, connectionString_{getConnectionString()}, conn_{connectionString_} {
  if (!conn_.is_open()) {
    throw std::runtime_error("Failed to open db");
  }
}

auto PostgresAdapter::readTickers(bool cache) -> Expected<Span<const TickerPrice>> {
  try {
    static std::vector<TickerPrice> tickers;
    if (!cache) {
      tickers.clear();
    } else if (!tickers.empty()) {
      return Span<const TickerPrice>{tickers};
    }
    pqxx::work transaction(conn_);
    transaction.exec("SET statement_timeout = 50");
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

auto PostgresAdapter::checkCredentials(CRef<String> name,
                                       CRef<String> password) -> Expected<ClientId> {
  LOG_DEBUG("Sensitive information, please look away. Authenticating {} {}", name, password);
  try {
    pqxx::work transaction(conn_);
    // Set small timeout, systemBus must be responsive.
    transaction.exec("SET statement_timeout = 50");
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

/**
 * @brief cicd compatibility
 */
String PostgresAdapter::getConnectionString() const {
  using namespace utils;
  const String host = getEnvVar("POSTGRES_HOST");
  const String port = getEnvVar("POSTGRES_PORT");
  const String user = getEnvVar("POSTGRES_USER");
  const String password = getEnvVar("POSTGRES_PASSWORD");
  const String dbName = getEnvVar("POSTGRES_DB");

  return std::format("host={} port={} user={} password={} dbname={} connect_timeout=1",
                     host.empty() ? config_.get<String>("db.host") : host,
                     port.empty() ? config_.get<String>("db.port") : port,
                     user.empty() ? config_.get<String>("db.user") : user,
                     password.empty() ? config_.get<String>("db.password") : password,
                     dbName.empty() ? config_.get<String>("db.dbname") : dbName);
}

} // namespace hft::adapters

#pragma GCC diagnostic pop