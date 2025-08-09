/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <filesystem>
#include <sstream>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "boost_types.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft {

struct Config {
  static void load(CRef<String> fileName) {
    if (fileName.empty() || !std::filesystem::exists(fileName)) {
      throw std::runtime_error(std::format("Config file not found {}", fileName));
    }
    file = fileName;
    boost::property_tree::read_ini(fileName, data);
  }

  template <typename Type>
  static Type get(CRef<String> name) {
    if (data.empty()) {
      throw std::runtime_error("Config has not been loaded");
    }
    try {
      return data.get<Type>(name);
    } catch (const std::exception &e) {
      throw std::runtime_error(std::format("std::exception {} {}", file, e.what()));
    }
  }

  template <typename Type>
  static auto get_optional(CRef<String> name) {
    if (data.empty()) {
      throw std::runtime_error("Config has not been loaded");
    }
    try {
      return data.get_optional<Type>(name);
    } catch (const std::exception &e) {
      throw std::runtime_error(std::format("std::exception {} {}", file, e.what()));
    }
  }

  inline static String file;
  inline static boost::property_tree::ptree data;
};

template <>
inline Vector<String> Config::get<Vector<String>>(CRef<String> name) {
  Vector<String> items;
  const auto value = Config::get<String>(name);

  std::stringstream ss(value);
  String item;
  while (std::getline(ss, item, ',')) {
    items.push_back(item);
  }
  return items;
}

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP