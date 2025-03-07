/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENT_HPP
#define HFT_SERVER_SERVEREVENT_HPP

#include <stdint.h>

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {
enum class ServerEvent : uint8_t { Initialized, CoresWarmedUp };
}

namespace hft::utils {
template <>
std::string toString<server::ServerEvent>(const server::ServerEvent &event) {
  using namespace server;
  switch (event) {
  case ServerEvent::Initialized:
    return "initialized";
  case ServerEvent::CoresWarmedUp:
    return "cores warmed up";
  default:
    return std::format("unknown server event {}", static_cast<uint8_t>(event));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVEREVENT_HPP