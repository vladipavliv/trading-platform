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
#include "container_types.hpp"
#include "domain_types.hpp"
#include "functional_types.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "table_reader.hpp"
#include "table_writer.hpp"

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

public:
  PostgresAdapter();

  auto readTickers(bool cache = true) -> Expected<Span<const TickerPrice>>;
  auto checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId>;
  auto getWriter(StringView table) -> Expected<TableWriter>;
  auto getReader(StringView table) -> Expected<TableReader>;
  void clean(CRef<String> table);

private:
  String getConnectionString() const;

private:
  const String connectionString_;
  pqxx::connection conn_;
};

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESADAPTER_HPP

#pragma GCC diagnostic pop