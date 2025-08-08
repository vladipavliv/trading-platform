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

#include "adapters/concepts/db_adapter_concept.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::adapters::impl {

/**
 * @brief Postgres adapter
 * @todo For better configurability and because adapters won't participate in hot paths,
 * its better to use interfaces here and make adapter factory.
 * @todo Currently adapters operate over the SystemBus, which is single-threaded, so
 * thread-safety of adapters is not a concern.
 * @todo For write/read, add some stream operations for TableReader and TableWriter.
 * So then it could go like this:
 *
 * auto stream = dbAdapter_.openTableStream("orders");
 * TableWriter writer{...};
 * stream << writer;
 * TableReader reader{...};
 * reader << stream;
 */
class PostgresAdapter {
  static constexpr auto SELECT_TICKERS_QUERY = "SELECT * FROM tickers";
  static constexpr auto TICKERS_COUNT_QUERY = "SELECT COUNT(*) FROM tickers";
  static constexpr auto SELECT_CLIENT_QUERY =
      "SELECT client_id, password FROM clients WHERE name = $1";

public:
  PostgresAdapter() : connectionString_{getConnectionString()}, conn_{connectionString_} {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open db");
    }
  }

  auto readTickers(bool cache = true) -> Expected<Vector<TickerPrice>> {
    try {
      static std::vector<TickerPrice> tickers;
      if (!cache) {
        tickers.clear();
      } else if (!tickers.empty()) {
        return tickers;
      }
      pqxx::work transaction(conn_);
      transaction.exec("SET statement_timeout = 1000");
      const pqxx::result countResult = transaction.exec(TICKERS_COUNT_QUERY);

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
      LOG_ERROR_SYSTEM("Exception during tickers read {}", e.what());
      return std::unexpected(StatusCode::DbError);
    }
  }

  auto checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId> {
    LOG_DEBUG("Sensitive information, please look away. Authenticating {} {}", name, password);
    try {
      pqxx::work transaction(conn_);
      // Set small timeout, systemBus must be responsive.
      // Maybe later on make a separate DataBus for such operations
      transaction.exec("SET statement_timeout = 50");
      const auto result = transaction.exec_params(SELECT_CLIENT_QUERY, name);

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

  template <TableWriterable TableWriter>
  bool write(TableWriter &writer) {
    try {
      pqxx::work transaction(conn_);
      const auto table = writer.table();

      const pqxx::table_path tablePath{std::string_view(table.c_str())};
      auto tableStream = pqxx::stream_to::table(transaction, tablePath);

      while (writer.next()) {
        const auto values = writer.get();
        tableStream << values;
      }
      tableStream.complete();
      transaction.commit();
      return true;
    } catch (CRef<pqxx::sql_error> e) {
      LOG_ERROR_SYSTEM("pqxx::sql_error {}", e.what());
      return false;
    } catch (CRef<std::exception> e) {
      LOG_ERROR_SYSTEM("std::exception {}", e.what());
      return false;
    }
  }

  template <TableReaderable TableReader>
  bool read(TableReader &reader) {
    try {
      pqxx::work transaction(conn_);
      const pqxx::result result = transaction.exec("SELECT * FROM " + reader.table() + ";");

      if (result.empty()) {
        return true;
      }

      reader.reserve(result.size());
      for (const auto &row : result) {
        std::vector<String> values;
        values.reserve(row.size());

        for (const auto &field : row) {
          if (field.is_null()) {
            values.push_back("");
          } else {
            values.push_back(field.as<String>());
          }
        }
        reader.set(values);
      }

      transaction.commit();
      return true;
    } catch (CRef<pqxx::sql_error> e) {
      LOG_ERROR_SYSTEM("pqxx::sql_error {}", e.what());
      return false;
    } catch (CRef<std::exception> e) {
      LOG_ERROR_SYSTEM("std::exception {}", e.what());
      return false;
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

} // namespace hft::adapters::impl

#endif // HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP

#pragma GCC diagnostic pop