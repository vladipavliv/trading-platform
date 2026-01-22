/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <memory>
#include <optional>

#include "utils/parse_utils.hpp"

namespace hft {

struct Config {
  explicit Config(const std::string &fileName) {
    if (!std::filesystem::exists(fileName)) {
      throw std::runtime_error("Config file not found: " + fileName);
    }
    boost::property_tree::read_ini(fileName, data);
  }

  template <typename T>
  T get(const std::string &name) const {
    return data.template get<T>(name);
  }

  template <typename T>
  std::vector<T> get_vector(const std::string &name) const {
    return utils::split<T>(get<std::string>(name));
  }

  template <typename T>
  auto get_optional(const std::string &name) const {
    return data.template get_optional<T>(name);
  }

private:
  boost::property_tree::ptree data;
};

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP