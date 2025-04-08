/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_DB_KAFKAPRODUCER_HPP
#define HFT_COMMON_DB_KAFKAPRODUCER_HPP

#include <librdkafka/rdkafkacpp.h>

#include "bus/bus.hpp"
#include "config/config.hpp"
#include "kafka_callback.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "serialization/flat_buffers/metadata_serializer.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Kafka adapter
 * @todo Make config for broker and topics, hardcoding for now
 */
template <typename SerializerType = serialization::fbs::MetadataSerializer>
class KafkaAdapter {
  using Conf = RdKafka::Conf;
  using Producer = RdKafka::Producer;
  using Topic = RdKafka::Topic;
  using ErrorCode = RdKafka::ErrorCode;

  static constexpr auto BROKER = "localhost:9092";
  static constexpr auto ORDER_TIMESTAMPS_TOPIC = "order-timestamps";

public:
  using Serializer = SerializerType;

  KafkaAdapter(SystemBus &bus) : bus_{bus} {
    LOG_DEBUG("Starting kafka adapter");
    conf_ = UPtr<Conf>(Conf::create(Conf::CONF_GLOBAL));

    if (conf_->set("dr_cb", &callback_, error_) != Conf::CONF_OK) {
      LOG_ERROR("Failed to set delivery report callback: {}", error_);
      throw std::runtime_error(error_);
    }

    if (conf_->set("metadata.broker.list", BROKER, error_) != Conf::CONF_OK) {
      LOG_ERROR("Failed to setup kafka brokers {}", error_);
      throw std::runtime_error(error_);
    }

    producer_ = UPtr<Producer>(Producer::create(conf_.get(), error_));
    if (!producer_) {
      LOG_ERROR("Failed to create producer {}", error_);
      throw std::runtime_error(error_);
    }

    createTopic(ORDER_TIMESTAMPS_TOPIC);

    bus_.subscribe<OrderTimestamp>(
        [this](CRef<OrderTimestamp> event) { produce(ORDER_TIMESTAMPS_TOPIC, event); });
  };

private:
  void createTopic(CRef<String> name) {
    LOG_DEBUG(name);
    if (topicMap_.count(name) != 0) {
      LOG_ERROR("Topic {} already exists", name);
      return;
    }
    auto topicPtr = UPtr<Topic>(Topic::create(producer_.get(), name, nullptr, error_));
    if (!topicPtr) {
      LOG_ERROR("Failed to create topic {}: {}", name, error_);
      throw std::runtime_error(error_);
    }
    topicMap_.insert(std::make_pair(name, std::move(topicPtr)));
  }

  template <typename MessageType>
  void produce(CRef<String> topic, CRef<MessageType> msg) {
    LOG_DEBUG("{} {}", topic, msg);
    // Kafka producer is thread-safe, as well as topic, as long as its not changed
    const auto topicIt = topicMap_.find(topic);
    if (topicIt == topicMap_.end()) {
      LOG_ERROR("Not connected to the topic {}", topic);
      return;
    }
    // librdkafka requires message be of a type void*
    auto serializedMsg = Serializer::serialize(msg);
    const ErrorCode result =
        producer_->produce(topicIt->second.get(), Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                           serializedMsg.data(), serializedMsg.size(), nullptr, nullptr);
    if (result != RdKafka::ERR_NO_ERROR) {
      LOG_ERROR("Failed to produce message {}", RdKafka::err2str(result));
    } else {
      LOG_DEBUG("Order sent to kafka");
    }
    producer_->poll(0);
  }

private:
  SystemBus &bus_;

  UPtr<Producer> producer_;
  UPtr<Conf> conf_;

  HashMap<String, UPtr<Topic>> topicMap_;

  String error_;
  KafkaCallback callback_;
};
} // namespace hft

#endif // HFT_COMMON_DB_KAFKAPRODUCER_HPP