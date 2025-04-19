/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_DB_KAFKAPRODUCER_HPP
#define HFT_COMMON_DB_KAFKAPRODUCER_HPP

#include <librdkafka/rdkafkacpp.h>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "kafka_callbacks.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "serialization/flat_buffers/metadata_serializer.hpp"

#include "types.hpp"

namespace hft {

struct KafkaConfig {
  const String broker;
  const String consumerGroup;
  const Milliseconds pollRate;
};

/**
 * @brief Kafka adapter
 * @details For message producing the type of each message is set explicitly
 * so adapter can subscribe for the messages in a SystemBus
 * For consuming serializer takes case of the types and routing to a SystemBus
 */
template <typename ConsumeSerializerType = serialization::fbs::MetadataSerializer,
          typename ProduceSerializerType = serialization::fbs::MetadataSerializer>
class KafkaAdapter {
  static constexpr auto BROKER = "localhost:9092";

public:
  using ConsumeSerializer = ConsumeSerializerType;
  using ProduceSerializer = ProduceSerializerType;

  KafkaAdapter(SystemBus &bus, CRef<KafkaConfig> config)
      : bus_{bus}, config_{config}, timer_{bus_.ioCtx} {
    LOG_DEBUG("Starting kafka adapter");
    createProducer();
    createConsumer();
  };

  ~KafkaAdapter() {
    consumer_->unsubscribe();
    while (producer_->outq_len() > 0) {
      LOG_INFO_SYSTEM("Flushing {} messages to kafka", producer_->outq_len());
      producer_->flush(1000);
    }
  }

  template <typename EventType>
  void addProduceTopic(CRef<String> topic) {
    using namespace RdKafka;
    LOG_DEBUG(topic);
    if (produceTopicMap_.count(topic) != 0) {
      LOG_ERROR("Topic already exist");
      return;
    }
    UPtr<Topic> topicPtr{Topic::create(producer_.get(), topic, nullptr, error_)};
    if (!topicPtr) {
      LOG_ERROR("Failed to create topic {}: {}", topic, error_);
      throw std::runtime_error(error_);
    }
    produceTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    bus_.subscribe<EventType>([this, topic](CRef<EventType> event) { produce(topic, event); });
  }

  void addConsumeTopic(CRef<String> topic) {
    LOG_DEBUG(topic);
    consumeTopics_.push_back(topic);
  }

  void start() {
    LOG_DEBUG("Starting kafka feed");
    if (state_ == State::Error) {
      throw std::runtime_error(error_);
    }
    state_ = State::On;
    const auto res = consumer_->subscribe(consumeTopics_);
    if (res != RdKafka::ERR_NO_ERROR) {
      const String errMsg = RdKafka::err2str(res);
      throw std::runtime_error(errMsg);
    }
  }

  void stop() {
    LOG_DEBUG("Stoping kafka feed");
    state_ = State::Off;
    const auto res = consumer_->unsubscribe();
    if (res != RdKafka::ERR_NO_ERROR) {
      const String errMsg = RdKafka::err2str(res);
      throw std::runtime_error(errMsg);
    }
  }

private:
  void createProducer() {
    using namespace RdKafka;
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", config_.broker, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("dr_cb", &deliveryCb_, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    producer_ = UPtr<Producer>(Producer::create(conf.get(), error_));
    if (!producer_) {
      throw std::runtime_error(error_);
    }
  }

  void createConsumer() {
    using namespace RdKafka;
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", config_.broker, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    if (conf->set("group.id", config_.consumerGroup, error_) != Conf::CONF_OK) {
      throw std::runtime_error(error_);
    }
    consumer_ = UPtr<KafkaConsumer>(KafkaConsumer::create(conf.get(), error_));
    if (!consumer_) {
      throw std::runtime_error(error_);
    }
  }

  void pollConsume() {
    using namespace RdKafka;
    const UPtr<Message> msg{consumer_->consume(0)};
    if (msg->err() == RdKafka::ERR_NO_ERROR) {
      ConsumeSerializer::deserialize(msg->payload(), msg->len(), bus_);
    } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
      LOG_ERROR("Failed to poll kafka messages: {}", static_cast<int32_t>(msg->err()));
    }
  }

  void schedulePoll() {
    timer_.expires_after(config_.pollRate);
    timer_.async_wait([this](CRef<BoostError> code) {
      if (code) {
        LOG_ERROR("{}", code.message());
        return;
      }
      pollConsume();
      producer_->poll(0);
      schedulePoll();
    });
  }

  template <typename MessageType>
  void produce(CRef<String> topic, CRef<MessageType> msg) {
    using namespace RdKafka;
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
    // TODO(self): instead of RK_MSG_COPY one can do RK_MSG_FREE, but kafka would take
    // ownership, so serialization should be done into a heap
    auto serializedMsg = ProduceSerializer::serialize(msg);
    const ErrorCode result =
        producer_->produce(topicIt->second.get(), Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                           serializedMsg.data(), serializedMsg.size(), nullptr, nullptr);
    if (result != RdKafka::ERR_NO_ERROR) {
      LOG_ERROR("Failed to produce message {}", RdKafka::err2str(result));
    } else {
      LOG_DEBUG("Order sent to kafka");
    }
  }

private:
  SystemBus &bus_;

  const KafkaConfig config_;

  UPtr<RdKafka::Producer> producer_;
  UPtr<RdKafka::KafkaConsumer> consumer_;

  HashMap<String, UPtr<RdKafka::Topic>> produceTopicMap_;
  std::vector<String> consumeTopics_;

  SteadyTimer timer_;

  State state_{State::Off};
  String error_;

  KafkaEventCallback eventCb_;
  KafkaDeliveryCallback deliveryCb_;
};
} // namespace hft

#endif // HFT_COMMON_DB_KAFKAPRODUCER_HPP