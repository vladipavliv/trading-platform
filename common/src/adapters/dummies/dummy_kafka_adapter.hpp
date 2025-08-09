/**
 * @author Vladimir Pavliv
 * @date 2025-08-07
 */

#ifndef HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP
#define HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "types.hpp"

namespace hft::adapters::impl {

template <Busable BusType, typename ConsumeSerializer = void, typename ProduceSerializer = void>
class DummyKafkaAdapter {
public:
  DummyKafkaAdapter(BusType &bus) { LOG_DEBUG("Kafka dummy adapter"); }

  void start() {}
  void stop() {}

  template <typename EventType>
  void produce(CRef<EventType> event, CRef<String> topic) {}

  template <typename EventType>
  void bindProduceTopic(CRef<String> topic) {}
};

} // namespace hft::adapters::impl

#endif // HFT_COMMON_ADAPTERS_DUMMYKAFKAADAPTER_HPP