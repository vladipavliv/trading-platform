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
    if (!std::filesystem::exists(fileName)) {
      throw std::runtime_error(std::format("{} does not exist in {}", fileName,
                                           std::filesystem::current_path().string()));
    }
    boost::property_tree::read_ini(fileName, data);
  }

  template <typename Type>
  static Type get(CRef<String> name) {
    return data.get<Type>(name);
  }

  template <typename Type>
  static auto get_optional(CRef<String> name) {
    return data.get_optional<Type>(name);
  }

  static boost::property_tree::ptree data;
};

boost::property_tree::ptree Config::data;

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP