/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <filesystem>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "boost_types.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft {

struct Config {
  static void load(CRef<String> fileName) {
    if (isLoaded()) {
      LOG_ERROR("Config is already loaded");
      return;
    }
    if (!std::filesystem::exists(fileName)) {
      throw std::runtime_error(std::format("Config file not found {}", fileName));
    }
    boost::property_tree::read_ini(fileName, data);
    loaded = true;
  }

  template <typename Type>
  static Type get(CRef<String> name) {
    return data.get<Type>(name);
  }

  template <typename Type>
  static auto get_optional(CRef<String> name) {
    return data.get_optional<Type>(name);
  }

  inline static bool isLoaded() { return loaded && !data.empty(); }

  inline static boost::property_tree::ptree data;
  inline static bool loaded{false};
};

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP