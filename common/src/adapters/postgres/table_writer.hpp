/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_ADAPTERS_POSTGRESTABLEWRITER_HPP
#define HFT_COMMON_ADAPTERS_POSTGRESTABLEWRITER_HPP

#include <pqxx/pqxx>
#include <pqxx/stream_to>

#include "types.hpp"

namespace hft::adapters {

class TableWriter {
public:
  TableWriter(pqxx::connection &conn, StringView table) : work_{conn}, stream_{work_, table} {}

  template <typename RowType>
  TableWriter &operator<<(const RowType &row) {
    stream_ << row;
    return *this;
  }

  void commit() {
    stream_.complete();
    work_.commit();
  }

private:
  pqxx::work work_;
  pqxx::stream_to stream_;
};
} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESTABLEWRITER_HPP