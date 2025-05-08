/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_UTILITIES_HPP
#define HFT_COMMON_UTILITIES_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cxxabi.h>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <typeinfo>

#include "boost_types.hpp"
#include "domain_types.hpp"
#include "types.hpp"

namespace hft::utils {

void pinThreadToCore(size_t coreId);
void setTheadRealTime();

auto generateOrderId() -> OrderId;
auto generateConnectionId() -> ConnectionId;
auto generateToken() -> Token;

auto getTimestamp() -> Timestamp;
auto createUdpSocket(IoCtx &ctx, bool broadcast = true, Port port = 0) -> UdpSocket;
auto split(CRef<String> input) -> ByteBuffer;

template <typename Type, typename... Types>
static constexpr bool contains = (std::is_same_v<Type, Types> || ...);

template <size_t First>
constexpr bool is_ascending() {
  return true;
}

template <size_t First, size_t Second, size_t... Rest>
constexpr bool is_ascending() {
  if constexpr (sizeof...(Rest) == 0) {
    return First <= Second;
  } else {
    return (First <= Second) && is_ascending<Second, Rest...>();
  }
}

template <typename ValueType>
bool hasIntersection(const std::vector<ValueType> &left, const std::vector<ValueType> &right) {
  return std::any_of(left.begin(), left.end(), [&](const ValueType &value) {
    return std::find(right.begin(), right.end(), value) != right.end();
  });
}

template <typename Number>
auto thousandify(Number input) -> String {
  std::stringstream ss;
  ss.imbue(std::locale("en_US.UTF-8"));
  ss << std::fixed << input;
  return ss.str();
}

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
