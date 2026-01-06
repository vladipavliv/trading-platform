/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "table_writer.hpp"

namespace hft::adapters {

TableWriter::TableWriter(pqxx::connection &conn, StringView table)
    : work_{conn}, stream_{work_, table} {}

void TableWriter::commit() {
  stream_.complete();
  work_.commit();
}

} // namespace hft::adapters

#pragma GCC diagnostic pop