/**
 * @author Vladimir Pavliv
 * @date 2025-07-07
 */

#ifndef HFT_COMMON_ADAPTERS_DBDAPTERCONCEPT_HPP
#define HFT_COMMON_ADAPTERS_DBDAPTERCONCEPT_HPP

#include <concepts>

#include "domain_types.hpp"
#include "types.hpp"

namespace hft::adapters {

/**
 * @brief Table writer to do bulk insert to a table
 * @details Needed when server/client specific types need to be written
 */
template <typename Writer>
concept TableWriterable = requires(Writer &writer) {
  { writer.table() } -> std::same_as<String>;
  { writer.get() } -> std::same_as<std::vector<String>>;
  { writer.next() } -> std::same_as<bool>;
};

/**
 * @brief Table reader to do bulk read from a table
 * @details Needed when server/client specific types need to be read
 */
template <typename Reader>
concept TableReaderable = requires(Reader &reader, CRef<std::vector<String>> row, size_t size) {
  { reader.table() } -> std::same_as<String>;
  { reader.set(row) } -> std::same_as<void>;
  { reader.reserve(size) } -> std::same_as<void>;
};

template <typename Adapter>
concept DbAdapterable =
    requires(Adapter &db, String &name, String &password, String &table, bool cache) {
      { db.readTickers(cache) } -> std::same_as<Expected<Vector<TickerPrice>>>;
      { db.checkCredentials(name, password) } -> std::same_as<Expected<ClientId>>;
      { db.clean(table) } -> std::same_as<void>;
      { db.write(std::declval<TableWriterable auto &>()) } -> std::same_as<bool>;
      { db.read(std::declval<TableReaderable auto &>()) } -> std::same_as<bool>;
    };

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_DBDAPTERCONCEPT_HPP