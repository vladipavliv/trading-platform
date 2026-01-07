/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP
#define HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <pqxx/pqxx>

#include "primitive_types.hpp"

namespace hft::adapters {
class TableReader {
public:
  TableReader(pqxx::connection &conn, StringView table);

  bool next();
  void commit();
  size_t size() const;
  bool empty() const;

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

private:
  String query(StringView table) const;

private:
  pqxx::work work_;
  pqxx::result result_;
  std::ptrdiff_t cursor_{-1};
};
} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_POSTGRESTABLEREADER_HPP

#pragma GCC diagnostic pop