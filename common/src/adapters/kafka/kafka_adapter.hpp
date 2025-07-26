/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP
#define HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP

#include <librdkafka/rdkafkacpp.h>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "config/config.hpp"
#include "kafka_callbacks.hpp"
#include "kafka_event.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "serialization/protobuf/proto_metadata_serializer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Kafka adapter
 * @details Reactive adapter, all communication goes through the bus
 * @todo Make separate DataBus, SystemBus should be responsive and not overwhelmed
 * by the kafka traffic. Even though priorities could be implemented in the system bus
 * so important events go ::dispatch instead of ::post, its better to make separate bus
 * with no responsiveness requirements.
 * If DataBus ends up multithreaded, this adapter should probably work fine,
 * but needs to be tested
 */
template <typename ConsumeSerializerType = serialization::ProtoMetadataSerializer,
          typename ProduceSerializerType = serialization::ProtoMetadataSerializer>
class KafkaAdapter {
public:
  using ConsumeSerializer = ConsumeSerializerType;
  using ProduceSerializer = ProduceSerializerType;

  explicit KafkaAdapter(SystemBus &bus)
      : bus_{bus}, enabled_{Config::get_optional<bool>("kafka.kafka_feed")},
        broker_{Config::get<String>("kafka.kafka_broker")},
        consumerGroup_{Config::get<String>("kafka.kafka_consumer_group")},
        pollRate_{Milliseconds(Config::get<int>("kafka.kafka_poll_rate"))}, timer_{bus_.ioCtx} {
    if (!enabled()) {
      LOG_DEBUG("Kafka is disabled");
      return;
    }
    LOG_DEBUG("Starting kafka adapter");
    createProducer();
    createConsumer();
  };

  ~KafkaAdapter() {
    if (!enabled()) {
      return;
    }
    consumer_->unsubscribe();
    while (producer_->outq_len() > 0) {
      LOG_INFO_SYSTEM("Flushing {} messages to kafka", producer_->outq_len());
      producer_->flush(1000);
    }
  }

  template <typename EventType>
  void addProduceTopic(CRef<String> topic) {
    if (!enabled()) {
      LOG_DEBUG("Kafka is disabled");
      return;
    }
    using namespace RdKafka;
    LOG_DEBUG("Adding produce topic {}", topic);
    if (produceTopicMap_.count(topic) != 0) {
      LOG_ERROR("Topic already exist");
      return;
    }
    UPtr<Topic> topicPtr{Topic::create(producer_.get(), topic, nullptr, error_)};
    if (!topicPtr) {
      onFatalError(std::format("Failed to create topic {}: {}", topic, error_));
      return;
    }
    produceTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    bus_.subscribe<EventType>([this, topic](CRef<EventType> event) { produce(topic, event); });
  }

  void addConsumeTopic(CRef<String> topic) {
    if (!enabled()) {
      LOG_DEBUG("Kafka is disabled");
      return;
    }
    LOG_DEBUG("Adding consume topic {}", topic);
    consumeTopics_.push_back(topic);
  }

  void start() {
    if (!enabled()) {
      LOG_DEBUG("Kafka is disabled");
      return;
    }
    LOG_DEBUG("Starting kafka feed");
    if (state_ != State::Off) {
      LOG_ERROR("Invalid state to start KafkaAdapter {}", error_);
      return;
    }
    state_ = State::On;
    const auto res = consumer_->subscribe(consumeTopics_);
    if (res != RdKafka::ERR_NO_ERROR) {
      onFatalError(RdKafka::err2str(res));
    }
  }

  void stop() {
    if (!enabled()) {
      LOG_DEBUG("Kafka is disabled");
      return;
    }
    if (state_ != State::On) {
      LOG_ERROR("Invalid state to stop KafkaAdapter {}", error_);
      return;
    }
    LOG_DEBUG("Stoping kafka feed");
    state_ = State::Off;
    const auto res = consumer_->unsubscribe();
    if (res != RdKafka::ERR_NO_ERROR) {
      onFatalError(RdKafka::err2str(res));
    }
  }

private:
  void createProducer() {
    using namespace RdKafka;
    if (producer_ != nullptr) {
      LOG_ERROR("Kafka producer is already created");
      return;
    }
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    if (conf->set("dr_cb", &deliveryCb_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    if (conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    producer_ = UPtr<Producer>(Producer::create(conf.get(), error_));
    if (!producer_) {
      onFatalError();
      return;
    }
  }

  void createConsumer() {
    using namespace RdKafka;
    if (consumer_ != nullptr) {
      LOG_ERROR("Kafka consumer is already created");
      return;
    }
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    if (conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    if (conf->set("group.id", consumerGroup_, error_) != Conf::CONF_OK) {
      onFatalError();
      return;
    }
    consumer_ = UPtr<KafkaConsumer>(KafkaConsumer::create(conf.get(), error_));
    if (!consumer_) {
      onFatalError();
      return;
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
    timer_.expires_after(pollRate_);
    timer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        onFatalError(code.message());
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
    LOG_TRACE("{} {}", topic, utils::toString(msg));
    if (state_ != State::On) {
      return;
    }
    // Both Kafka producer and Topic are thread-safe, as long as they are not changed
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
      onFatalError(RdKafka::err2str(result));
    }
  }

  void onFatalError(CRef<String> error = "") {
    if (!error.empty()) {
      error_ = error;
    }
    LOG_ERROR_SYSTEM("{}", error_);
    state_ = State::Error;
    bus_.post(KafkaEvent{KafkaStatus::Error, error_});
  }

  bool enabled() const { return enabled_.has_value() && enabled_.value(); }

private:
  SystemBus &bus_;

  const std::optional<bool> enabled_;
  const String broker_;
  const String consumerGroup_;
  const Milliseconds pollRate_;

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

#endif // HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP