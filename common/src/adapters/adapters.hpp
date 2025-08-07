/**
 * @author Vladimir Pavliv
 * @date 2025-08-07
 */

#ifndef HFT_COMMON_ADAPTERS_ADAPTERFACTORY_HPP
#define HFT_COMMON_ADAPTERS_ADAPTERFACTORY_HPP

#ifdef TELEMETRY_ENABLED
#include "adapters/kafka/kafka_adapter.hpp"
#else
#include "dummies/dummy_kafka_adapter.hpp"
#endif
#include "adapters/postgres/postgres_adapter.hpp"
#include "bus/system_bus.hpp"

namespace hft::adapters {

#ifdef TELEMETRY_ENABLED
template <typename ConsumeSerializer = serialization::ProtoMetadataSerializer,
          typename ProduceSerializer = serialization::ProtoMetadataSerializer>
using MessageQueueAdapter = impl::KafkaAdapter<ConsumeSerializer, ProduceSerializer>;
#else
template <typename ConsumeVoid = void, typename ProduceVoid = void>
using MessageQueueAdapter = impl::DummyKafkaAdapter<ConsumeVoid, ProduceVoid>;
#endif
using DbAdapter = impl::PostgresAdapter;

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_ADAPTERFACTORY_HPP