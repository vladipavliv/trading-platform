/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "boost_types.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft {

class Config {
public:
  static void load(CRef<String> fileName) {
    boost::property_tree::read_ini(fileName, propertyTree_);
  }

  template <typename PropertyType>
  Expected<PropertyType> get(CRef<String> name) {
    try {
      return propertyTree_.get<PropertyType>(name, PropertyType{});
    } catch (const boost::property_tree::ptree_error &e) {
      LOG_ERROR_SYSTEM("Failed to read property {} error: {}", name, e.what());
      return std::unexpected(e.what());
    }
  }

private:
  static boost::property_tree::ptree propertyTree_;
};

boost::property_tree::ptree Config::propertyTree_;

} // namespace hft

#endif // HFT_COMMON_CONFIG_HPP