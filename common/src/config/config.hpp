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
      throw std::runtime_error(std::format("Config file not found {} {}",
                                           std::filesystem::current_path().c_str(), fileName));
    }
    file = fileName;
    boost::property_tree::read_ini(fileName, data);
  }

  template <typename Type>
  static Type get(CRef<String> name) {
    if (data.empty()) {
      throw std::runtime_error(std::format("Failed to get {}: config has not been loaded", name));
    }

    if constexpr (requires { typename Type::value_type; } && !std::is_same_v<Type, String>) {
      Type items;
      std::stringstream ss(data.get<String>(name));
      using ValueType = typename Type::value_type;

      if constexpr (std::is_arithmetic_v<ValueType>) {

        using ParseType = std::conditional_t<(sizeof(ValueType) <= 1), int, ValueType>;
        ParseType tempVal;

        while (ss >> tempVal) {
          items.push_back(static_cast<ValueType>(tempVal));
          while (ss.peek() == ',' || ss.peek() == ' ')
            ss.ignore();
        }
      } else {
        String item;
        while (std::getline(ss, item, ',')) {
          items.push_back(item);
        }
      }
      return items;
    } else {
      return data.get<Type>(name);
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

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP