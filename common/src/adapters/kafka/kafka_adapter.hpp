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
 * @todo Make config for broker and topics, hardcoding for now
 */
template <typename ConsumeSerializerType,
          typename ProduceSerializerType = serialization::fbs::MetadataSerializer>
class KafkaAdapter {
  using Conf = RdKafka::Conf;
  using Producer = RdKafka::Producer;
  using Consumer = RdKafka::Consumer;
  using Topic = RdKafka::Topic;
  using Message = RdKafka::Message;
  using ErrorCode = RdKafka::ErrorCode;

  static constexpr auto BROKER = "localhost:9092";
  static constexpr auto ORDER_TIMESTAMPS_TOPIC = "order-timestamps";

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

  struct Config {
    const String broker;
    const String consumerGroup;
    const std::vector<String> produceTopics;
    const std::vector<String> consumeTopics;
  };

  KafkaAdapter(SystemBus &bus, CRef<Config> config) : bus_{bus}, config_{config} {
    LOG_DEBUG("Starting kafka adapter");
    createProducer();
    createConsumer();
    createTopics();

    bus_.subscribe<OrderTimestamp>(
        [this](CRef<OrderTimestamp> event) { produce(ORDER_TIMESTAMPS_TOPIC, event); });
  };

  void start() {
    LOG_DEBUG("Starting kafka feed");
    if (state_ != State::Error) {
      state_ = State::On;
    }
    for (auto &topic : consumeTopicMap_) {
      const auto res = consumer_->start(topic.second.get(), Topic::PARTITION_UA, 0);
      if (res != RdKafka::ERR_NO_ERROR) {
        LOG_ERROR_SYSTEM("Failed to subscribe to {} {} ", topic.first, RdKafka::err2str(res));
      }
    }
  }

  void stop() {
    LOG_DEBUG("Stoping kafka feed");
    if (state_ != State::Error) {
      state_ = State::Off;
    }
    for (auto &topic : consumeTopicMap_) {
      const auto res = consumer_->stop(topic.second.get(), Topic::PARTITION_UA);
      if (res != RdKafka::ERR_NO_ERROR) {
        LOG_ERROR_SYSTEM("Failed to unsubscribe from {} {} ", topic.first, RdKafka::err2str(res));
      }
    }
  }

  void poll(size_t timeout = 0) {
    LOG_TRACE("Polling messages from kafka");
    for (auto &topic : consumeTopicMap_) {
      const auto msg =
          UPtr<Message>(consumer_->consume(topic.second.get(), Topic::PARTITION_UA, timeout));
      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        ConsumeSerializer::deserialize(msg->payload(), msg->len(), bus_);
      } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
        LOG_ERROR("Failed to poll kafka messages: {}", static_cast<int32_t>(msg->err()));
      }
    }
  }

private:
  void createProducer() {
    UPtr<Conf> conf = UPtr<Conf>(Conf::create(Conf::CONF_GLOBAL));
    if (conf->set("bootstrap.servers", config_.broker, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("dr_cb", &callback_, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    producer_ = UPtr<Producer>(Producer::create(conf.get(), error_));
    if (!producer_) {
      throw std::runtime_error(error_);
    }
  }

  void createConsumer() {
    UPtr<Conf> conf = UPtr<Conf>(Conf::create(Conf::CONF_GLOBAL));
    if (conf->set("bootstrap.servers", config_.broker, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("group.id", config_.consumerGroup, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    consumer_ = UPtr<Consumer>(RdKafka::Consumer::create(conf.get(), error_));
    if (!consumer_) {
      throw std::runtime_error(error_);
    }

    for (auto &topic : config_.consumeTopics) {
      if (consumeTopicMap_.count(topic) != 0) {
        LOG_ERROR("Duplicate topic");
        continue;
      }
      auto topicPtr = UPtr<Topic>(Topic::create(consumer_.get(), topic, nullptr, error_));
      if (!topicPtr) {
        LOG_ERROR("Failed to create topic {}: {}", topic, error_);
        throw std::runtime_error(error_);
      }
      consumeTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    }
  }

  void createTopics() {
    for (auto &topic : config_.produceTopics) {
      if (produceTopicMap_.count(topic) != 0) {
        LOG_ERROR("Duplicate topic");
        continue;
      }
      auto topicPtr = UPtr<Topic>(Topic::create(producer_.get(), topic, nullptr, error_));
      if (!topicPtr) {
        LOG_ERROR("Failed to create topic {}: {}", topic, error_);
        throw std::runtime_error(error_);
      }
      produceTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    }
    for (auto &topic : config_.consumeTopics) {
      if (consumeTopicMap_.count(topic) != 0) {
        LOG_ERROR("Duplicate topic");
        continue;
      }
      auto topicPtr = UPtr<Topic>(Topic::create(consumer_.get(), topic, nullptr, error_));
      if (!topicPtr) {
        LOG_ERROR("Failed to create topic {}: {}", topic, error_);
        throw std::runtime_error(error_);
      }
      consumeTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
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
  const Config config_;

  UPtr<Producer> producer_;
  UPtr<Consumer> consumer_;

  HashMap<String, UPtr<Topic>> produceTopicMap_;
  HashMap<String, UPtr<Topic>> consumeTopicMap_;

  State state_{State::Off};
  String error_;
  KafkaCallback callback_;
};
} // namespace hft

#endif // HFT_COMMON_DB_KAFKAPRODUCER_HPP