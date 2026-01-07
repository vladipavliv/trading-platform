/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <boost/property_tree/ptree_fwd.hpp>
#include <optional>
#include <vector>

#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/parse_utils.hpp"

namespace hft {

struct Config {
  static void load(CRef<String> fileName);

  template <typename Type>
  static Type get(CRef<String> name) {
    if constexpr (requires { typename Type::value_type; } && !std::is_same_v<Type, String>) {
      using VType = typename Type::value_type;
      return utils::split<VType>(getString(name));
    } else {
      return getImpl<Type>(name);
    }
  }

  template <typename Type>
  static std::optional<Type> get_optional(CRef<String> name) {
    if (!data)
      return std::nullopt;
    return getOptImpl<Type>(name);
  }

private:
  static String getString(CRef<String> name);

  template <typename Type>
  static Type getImpl(CRef<String> name);

  template <typename Type>
  static std::optional<Type> getOptImpl(CRef<String> name);

  inline static String file;
  inline static boost::property_tree::ptree *data{nullptr};
};

template <>
int Config::getImpl<int>(CRef<String> name);
template <>
uint32_t Config::getImpl<uint32_t>(CRef<String> name);
template <>
uint64_t Config::getImpl<uint64_t>(CRef<String> name);
template <>
String Config::getImpl<String>(CRef<String> name);
template <>
bool Config::getImpl<bool>(CRef<String> name);
template <>
uint64_t Config::getImpl<uint64_t>(CRef<String> name);
template <>
std::optional<uint64_t> Config::getOptImpl<uint64_t>(CRef<String> name);
template <>
std::optional<bool> Config::getOptImpl<bool>(CRef<String> name);

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP