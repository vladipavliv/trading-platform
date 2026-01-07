/**
 * @author Vladimir Pavliv
 * @date 2025-08-07
 */

#ifndef HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP
#define HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP

#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft::adapters {

template <typename BusT>
class DummyKafkaAdapter {
public:
  DummyKafkaAdapter(BusT &bus) { LOG_DEBUG("Kafka dummy adapter"); }

  void start() {}
  void stop() {}

  template <typename EventType>
  void produce(CRef<EventType> event, CRef<String> topic) {}

  template <typename EventType>
  void bindProduceTopic(CRef<String> topic) {}
};

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP