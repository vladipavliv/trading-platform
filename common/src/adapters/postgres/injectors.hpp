/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_ADAPTERS_INJECTORS_HPP
#define HFT_COMMON_ADAPTERS_INJECTORS_HPP

#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

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

} // namespace hft

#endif // HFT_COMMON_ADAPTERS_INJECTORS_HPP