/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_DB_KAFKAPRODUCER_HPP
#define HFT_COMMON_DB_KAFKAPRODUCER_HPP

#include <librdkafka/rdkafkacpp.h>

#include "bus/bus.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "serialization/flat_buffers/metadata_serializer.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Kafka adapter
 * @details For producing we define the type of the message so it can subscribe to system bus
 * For consuming Serializer does the type routing directly to the bus.
 */
template <typename ConsumeSerializerType,
          typename ProduceSerializerType = serialization::fbs::MetadataSerializer>
class KafkaAdapter {
  static constexpr auto BROKER = "localhost:9092";

  class KafkaCallback : public RdKafka::DeliveryReportCb {
  public:
    void dr_cb(RdKafka::Message &message) override {
      if (message.err()) {
        LOG_ERROR("Kafka delivery failed: {}", message.errstr());
      } else {
        LOG_TRACE("Message delivered to the topic: {}", message.topic_name());
      }
    }
  };

public:
  using ConsumeSerializer = ConsumeSerializerType;
  using ProduceSerializer = ProduceSerializerType;

  KafkaAdapter(SystemBus &bus, CRef<String> broker, CRef<String> consumerGroup)
      : bus_{bus}, broker_{broker}, consumerGroup_{consumerGroup} {
    LOG_DEBUG("Starting kafka adapter");
    createProducer();
    createConsumer();
  };

  template <typename EventType>
  void addProduceTopic(CRef<String> topic) {
    LOG_DEBUG(topic);
    if (produceTopicMap_.count(topic) != 0) {
      LOG_ERROR("Topic already exist");
      return;
    }
    UPtr<RdKafka::Topic> topicPtr{RdKafka::Topic::create(producer_.get(), topic, nullptr, error_)};
    if (!topicPtr) {
      LOG_ERROR("Failed to create topic {}: {}", topic, error_);
      throw std::runtime_error(error_);
    }
    produceTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    bus_.subscribe<EventType>([this, topic](CRef<OrderTimestamp> event) { produce(topic, event); });
  }

  void addConsumeTopic(CRef<String> topic) {
    LOG_DEBUG(topic);
    if (consumeTopicMap_.count(topic) != 0) {
      LOG_ERROR("Topic already exist");
      return;
    }
    UPtr<RdKafka::Topic> topicPtr{RdKafka::Topic::create(consumer_.get(), topic, nullptr, error_)};
    if (!topicPtr) {
      LOG_ERROR("Failed to create topic {}: {}", topic, error_);
      throw std::runtime_error(error_);
    }
    consumeTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
  }

  void start() {
    LOG_DEBUG("Starting kafka feed");
    if (state_ == State::Error) {
      throw std::runtime_error(error_);
    }
    state_ = State::On;
    for (auto &topic : consumeTopicMap_) {
      const auto res = consumer_->start(topic.second.get(), RdKafka::Topic::PARTITION_UA, 0);
      if (res != RdKafka::ERR_NO_ERROR) {
        const String errMsg = RdKafka::err2str(res);
        LOG_ERROR_SYSTEM("Failed to subscribe to {} {} ", topic.first, errMsg);
        throw std::runtime_error(errMsg);
      }
    }
  }

  void stop() {
    LOG_DEBUG("Stoping kafka feed");
    if (state_ == State::Error) {
      throw std::runtime_error(error_);
    }
    state_ = State::Off;
    for (auto &topic : consumeTopicMap_) {
      const auto res = consumer_->stop(topic.second.get(), RdKafka::Topic::PARTITION_UA);
      if (res != RdKafka::ERR_NO_ERROR) {
        const String errMsg = RdKafka::err2str(res);
        LOG_ERROR_SYSTEM("Failed to unsubscribe from {} {} ", topic.first, errMsg);
        throw std::runtime_error(errMsg);
      }
    }
  }

  void poll(size_t timeout = 0) {
    LOG_TRACE("Polling messages from kafka");
    for (auto &topic : consumeTopicMap_) {
      const UPtr<RdKafka::Message> msg{
          consumer_->consume(topic.second.get(), RdKafka::Topic::PARTITION_UA, timeout)};
      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        ConsumeSerializer::deserialize(msg->payload(), msg->len(), bus_);
      } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
        LOG_ERROR("Failed to poll kafka messages: {}", static_cast<int32_t>(msg->err()));
      }
    }
  }

private:
  void createProducer() {
    UPtr<RdKafka::Conf> conf{RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != RdKafka::Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("dr_cb", &callback_, error_) != RdKafka::Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    producer_ = UPtr<RdKafka::Producer>(RdKafka::Producer::create(conf.get(), error_));
    if (!producer_) {
      throw std::runtime_error(error_);
    }
  }

  void createConsumer() {
    UPtr<RdKafka::Conf> conf{RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != RdKafka::Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("group.id", consumerGroup_, error_) != RdKafka::Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    consumer_ = UPtr<RdKafka::Consumer>(RdKafka::Consumer::create(conf.get(), error_));
    if (!consumer_) {
      throw std::runtime_error(error_);
    }
  }

  template <typename MessageType>
  void produce(CRef<String> topic, CRef<MessageType> msg) {
    LOG_TRACE("{} {}", topic, msg);
    if (state_ != State::On) {
      return;
    }
    // Kafka producer is thread-safe, as well as topic, as long as its not changed
    const auto topicIt = produceTopicMap_.find(topic);
    if (topicIt == produceTopicMap_.end()) {
      LOG_ERROR("Not connected to the topic {}", topic);
      return;
    }
    // librdkafka requires message be of a type void*
    auto serializedMsg = ProduceSerializer::serialize(msg);
    const RdKafka::ErrorCode result = producer_->produce(
        topicIt->second.get(), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY,
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

  const String broker_;
  const String consumerGroup_;

  UPtr<RdKafka::Producer> producer_;
  UPtr<RdKafka::Consumer> consumer_;

  HashMap<String, UPtr<RdKafka::Topic>> produceTopicMap_;
  HashMap<String, UPtr<RdKafka::Topic>> consumeTopicMap_;

  State state_{State::Off};
  String error_;
  KafkaCallback callback_;
};
} // namespace hft

#endif // HFT_COMMON_DB_KAFKAPRODUCER_HPP