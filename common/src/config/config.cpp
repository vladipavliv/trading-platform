/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#include "config.hpp"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>

namespace hft {
void Config::load(CRef<String> fileName) {
  if (!std::filesystem::exists(fileName)) {
    throw std::runtime_error("Config file not found: " + fileName);
  }

  file = fileName;
  if (!data) {
    data = new boost::property_tree::ptree();
  }
  boost::property_tree::read_ini(fileName, *data);
}

String Config::getString(CRef<String> name) { return data->get<String>(name); }

// Specializations for get_impl to handle the actual Boost access
template <>
int Config::getImpl<int>(CRef<String> name) {
  return data->get<int>(name);
}

template <>
uint32_t Config::getImpl<uint32_t>(CRef<String> name) {
  return data->get<uint32_t>(name);
}

template <>
uint64_t Config::getImpl<uint64_t>(CRef<String> name) {
  return data->get<uint64_t>(name);
}

template <>
String Config::getImpl<String>(CRef<String> name) {
  return data->get<String>(name);
}

template <>
bool Config::getImpl<bool>(CRef<String> name) {
  return data->get<bool>(name);
}

template <>
std::optional<int> Config::getOptImpl<int>(CRef<String> name) {
  auto res = data->get_optional<int>(name);
  return res ? std::optional<int>(*res) : std::nullopt;
}

template <>
std::optional<String> Config::getOptImpl<String>(CRef<String> name) {
  auto res = data->get_optional<String>(name);
  return res ? std::optional<String>(*res) : std::nullopt;
}

template <>
std::optional<uint64_t> Config::getOptImpl<uint64_t>(CRef<String> name) {
  auto res = data->get_optional<uint64_t>(name);
  return res ? std::optional<uint64_t>(*res) : std::nullopt;
}

template <>
std::optional<bool> Config::getOptImpl<bool>(CRef<String> name) {
  auto res = data->get_optional<bool>(name);
  return res ? std::optional<bool>(*res) : std::nullopt;
}

} // namespace hft
