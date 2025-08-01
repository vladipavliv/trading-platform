/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_TABLEREADER_HPP
#define HFT_SERVER_TABLEREADER_HPP

#include "db_type_mapper.hpp"
#include "types.hpp"

namespace hft::server {

template <typename ElementType>
class TableReader {
public:
  explicit TableReader(CRef<String> table) : table_{table} {}

  auto table() const -> String { return table_; }

  void set(CRef<std::vector<String>> row) {
    result_.push_back(DbTypeMapper<ElementType>::fromRow(row));
  }

  void reserve(size_t size) { result_.reserve(size); }

  auto result() const -> CRef<std::vector<ElementType>> { return result_; }

private:
  const String table_;
  std::vector<ElementType> result_;
};

} // namespace hft::server

#endif // HFT_SERVER_TABLEREADER_HPP