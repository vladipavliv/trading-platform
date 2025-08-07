/**
 * @author Vladimir Pavliv
 * @date 2025-07-07
 */

#ifndef HFT_COMMON_ADAPTERS_MESSAGEQUEUEAPTERCONCEPT_HPP
#define HFT_COMMON_ADAPTERS_MESSAGEQUEUEAPTERCONCEPT_HPP

#include <concepts>

#include "types.hpp"

namespace hft::adapters {

template <typename Adapter>
concept MessageQueueAdapterable = [] {
  struct ProbeType {};
  String topic;
  return requires(Adapter &adapter) {
    { adapter.start() } -> std::same_as<void>;
    { adapter.stop() } -> std::same_as<void>;
    {
      adapter.template produce<ProbeType>(std::declval<CRef<String>>(), topic)
    } -> std::same_as<void>;
  };
}();

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_MESSAGEQUEUEAPTERCONCEPT_HPP