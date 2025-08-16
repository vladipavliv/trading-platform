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
    if (++cursor_ < static_cast<std::ptrdiff_t>(result_.size())) {
      return true;
    }
    return false;
  }

  template <typename ValueType>
  ValueType get(size_t col) const {
    if (cursor_ < 0 || cursor_ >= static_cast<std::ptrdiff_t>(result_.size())) {
      throw std::out_of_range("Row index out of range");
    }
    if (col >= result_[cursor_].size()) {
      throw std::out_of_range("Column index out of range");
    }
    return result_[cursor_][col].as<ValueType>();
  }

  void commit() { work_.commit(); }

  size_t size() const { return result_.size(); }

  bool empty() const { return result_.empty(); }

private:
  String query(StringView table) const { return std::format("SELECT * FROM {};", table); }

private:
  pqxx::work work_;
  pqxx::result result_;
  std::ptrdiff_t cursor_{-1};
};
} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP
