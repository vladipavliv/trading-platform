/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include "logging.hpp"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <memory>
#include <optional>

namespace hft {

struct Config {
  static void load(const std::string &fileName) {
    if (!std::filesystem::exists(fileName)) {
      throw std::runtime_error("Config file not found: " + fileName);
    }
    if (!data) {
      data = std::make_unique<boost::property_tree::ptree>();
    }
    boost::property_tree::read_ini(fileName, *data);
  }

  template <typename T>
  static T get(const std::string &name) {
    try {
      return data->template get<T>(name);
    } catch (const std::exception &e) {
      LOG_ERROR("Config key '{}' error: {}", name, e.what());
      throw;
    }
  }

  template <typename T>
  static std::optional<T> get_optional(const std::string &name) {
    if (!data)
      return std::nullopt;

    if (auto res = data->template get_optional<T>(name)) {
      return *res;
    }
    return std::nullopt;
  }

private:
  inline static std::unique_ptr<boost::property_tree::ptree> data{nullptr};
};

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP