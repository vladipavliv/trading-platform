/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP
#define HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP

#include <pqxx/pqxx>

#include "types.hpp"

namespace hft::adapters {
class TableReader {
public:
  TableReader(pqxx::connection &conn, StringView table)
      : work_{conn}, result_(work_.exec(query(table))) {}

  bool next() {
    if (cursor_ < result_.size()) {
      ++cursor_;
      return true;
    }
    return false;
  }

  template <typename ValueType>
  ValueType get(size_t col) const {
    if (col >= result_[cursor_].size()) {
      throw std::out_of_range("Column index out of range");
    }
    return result_[cursor_][col].as<ValueType>();
  }

  void commit() { work_.commit(); }

  size_t size() const { return result_.size(); }

  bool empty() const { return cursor_ >= result_.size(); }

private:
  String query(StringView table) const { return std::format("SELECT * FROM {};", table); }

private:
  pqxx::work work_;
  pqxx::result result_;
  size_t cursor_{0};
};
} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP