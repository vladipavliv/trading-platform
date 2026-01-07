/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <pqxx/pqxx>

#include "primitive_types.hpp"
#include "table_reader.hpp"

namespace hft::adapters {

TableReader::TableReader(pqxx::connection &conn, StringView table)
    : work_{conn}, result_(work_.exec(query(table))) {}

bool TableReader::next() {
  if (++cursor_ < static_cast<std::ptrdiff_t>(result_.size())) {
    return true;
  }
  return false;
}

void TableReader::commit() { work_.commit(); }

size_t TableReader::size() const { return result_.size(); }

bool TableReader::empty() const { return result_.empty(); }

String TableReader::query(StringView table) const {
  return std::format("SELECT * FROM {};", table);
}

} // namespace hft::adapters

#pragma GCC diagnostic pop