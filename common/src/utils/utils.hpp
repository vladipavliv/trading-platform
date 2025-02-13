/**
 * @file utils.hpp
 * @brief Helper functions and stuff
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_UTILITIES_HPP
#define HFT_COMMON_UTILITIES_HPP

#include <atomic>
#include <cstdint>
#include <cxxabi.h>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <typeinfo>

#include "market_types.hpp"
#include "network_types.hpp"
#include "types.hpp"

namespace hft::utils {

/**
 * @brief Fast and reliable id for local objects
 */
template <typename Type>
inline uintptr_t getId(const Type *obj) {
  return reinterpret_cast<uintptr_t>(obj);
}

void pinThreadToCore(int core_id);

void setTheadRealTime();

template <typename Type>
String getTypeName() {
  int status = 0;
  std::unique_ptr<char, decltype(&free)> demangled(
      abi::__cxa_demangle(typeid(Type).name(), nullptr, nullptr, &status), free);
  return (status == 0 && demangled) ? demangled.get() : typeid(Type).name();
}

size_t getTraderId(const TcpSocket &sock);

inline size_t generateOrderId() {
  static std::atomic<size_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
};

bool socketOk(const TcpSocket &sock);

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP