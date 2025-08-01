/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_TABLEWRITER_HPP
#define HFT_SERVER_TABLEWRITER_HPP

#include "db_type_mapper.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft::server {

template <typename ElementType>
class TableWriter {
public:
  TableWriter(CRef<String> table, CRef<std::vector<ElementType>> values)
      : table_{table}, values_{values} {}

  auto table() const -> String { return table_; }

  auto get() const -> std::vector<String> {
    if (index_ == 0 || index_ > values_.size()) {
      LOG_ERROR_SYSTEM("Index out of bounds in TableWriter");
      return {};
    }
    return DbTypeMapper<ElementType>::toRow(values_[index_ - 1]);
  }

  bool next() { return index_++ < values_.size(); }

private:
  const String table_;
  CRef<std::vector<ElementType>> values_;

  size_t index_{0};
};

} // namespace hft::server

#endif // HFT_SERVER_TABLEWRITER_HPP