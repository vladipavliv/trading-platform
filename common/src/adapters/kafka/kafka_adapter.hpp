/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP
#define HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP

#include <librdkafka/rdkafkacpp.h>
#include <sstream>

#include "boost_types.hpp"
#include "bus/busable.hpp"
#include "config/config.hpp"
#include "kafka_callbacks.hpp"
#include "kafka_event.hpp"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "serialization/proto/proto_metadata_serializer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::adapters {

/**
 * @brief Reactive adapter, all communication goes through the bus
 */
template <typename BusT, typename ConsumeSerializerT, typename ProduceSerializerT>
class KafkaAdapter {
  static_assert(Busable<BusT>, "BusT must satisfy the Busable concept");

  static constexpr size_t FLUSH_CHUNK_TIMEOUT = 1000;
  static constexpr size_t CONSUME_CHUNK = 100;

public:
  explicit KafkaAdapter(BusT &bus)
      : bus_{bus}, broker_{Config::get<String>("kafka.broker")},
        consumerGroup_{Config::get<String>("kafka.consumer_group")},
        pollRate_{Microseconds(Config::get<size_t>("kafka.poll_rate_us"))},
        consumeTopics_{Config::get<Vector<String>>("kafka.consume_topics")},
        produceTopics_{Config::get<Vector<String>>("kafka.produce_topics")},
        timer_{bus_.streamIoCtx()} {
    LOG_DEBUG("Kafka adapter ctor");
  };

  ~KafkaAdapter() {
    LOG_DEBUG("Kafka adapter dtor");
    stop();
  }

  bool start() {
    LOG_DEBUG("Start kafka adapter");
    if (started_) {
      LOG_ERROR("Kafka is already running");
      return false;
    }
    printTopics();
    if (!createConsumer()) {
      LOG_ERROR("Failed to create kafka consumer");
      return false;
    }
    if (!createProducer()) {
      consumer_.reset();
      LOG_ERROR("Failed to create kafka producer");
      return false;
    }
    if (!startConsume() || !startProduce()) {
      consumer_.reset();
      producer_.reset();
      LOG_ERROR("Failed to start kafka");
      return false;
    }
    started_ = true;
    schedulePoll();
    return true;
  }

  void stop() {
    LOG_DEBUG("Stop kafka adapter");
    started_ = false;
    timer_.cancel();

    if (consumer_ != nullptr) {
      consumer_->unsubscribe();
    }
    while (producer_ != nullptr && producer_->outq_len() > 0) {
      LOG_INFO_SYSTEM("Flushing {} messages to kafka", producer_->outq_len());
      producer_->flush(FLUSH_CHUNK_TIMEOUT);
    }
    produceTopicMap_.clear();
  }

  template <typename EventType>
  void produce(CRef<EventType> event, CRef<String> topic) {
    using namespace RdKafka;
    LOG_DEBUG("Producing message: {} to topic: {}", utils::toString(event), topic);
    const auto topicIt = produceTopicMap_.find(topic);
    if (topicIt == produceTopicMap_.end()) {
      LOG_ERROR("Not connected to the topic {}", topic);
      return;
    }
    auto serializedMsg = ProduceSerializerT::serialize(event);
    const ErrorCode result =
        producer_->produce(topicIt->second.get(), Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                           serializedMsg.data(), serializedMsg.size(), nullptr, nullptr);
    if (result != RdKafka::ERR_NO_ERROR) {
      onFatalError(RdKafka::err2str(result));
    }
  }

  template <typename EventType>
  void bindProduceTopic(CRef<String> topic) {
    bus_.template subscribe<EventType>([this, topic](CRef<EventType> msg) { produce(msg, topic); });
  }

private:
  bool createConsumer() {
    LOG_DEBUG("Create consumer");
    using namespace RdKafka;
    if (consumer_ != nullptr) {
      LOG_ERROR("Kafka consumer is already created");
      return false;
    }
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != Conf::CONF_OK ||
        conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK ||
        conf->set("group.id", consumerGroup_, error_) != Conf::CONF_OK ||
        conf->set("fetch.wait.max.ms", "10", error_) != Conf::CONF_OK ||
        conf->set("fetch.min.bytes", "1", error_) != Conf::CONF_OK ||
        conf->set("session.timeout.ms", "6000", error_) != Conf::CONF_OK ||
        conf->set("heartbeat.interval.ms", "2000", error_) != Conf::CONF_OK) {
      onFatalError();
      return false;
    }
    consumer_ = UPtr<KafkaConsumer>(KafkaConsumer::create(conf.get(), error_));
    if (!consumer_) {
      onFatalError();
      return false;
    }
    return true;
  }

  bool createProducer() {
    LOG_DEBUG("Create producer");
    using namespace RdKafka;
    if (producer_ != nullptr) {
      LOG_ERROR("Kafka producer is already created");
      return false;
    }
    UPtr<Conf> conf{Conf::create(Conf::CONF_GLOBAL)};
    if (conf->set("bootstrap.servers", broker_, error_) != Conf::CONF_OK ||
        conf->set("dr_cb", &deliveryCb_, error_) != Conf::CONF_OK ||
        conf->set("event_cb", &eventCb_, error_) != Conf::CONF_OK) {
      onFatalError();
      return false;
    }
    producer_ = UPtr<Producer>(Producer::create(conf.get(), error_));
    if (!producer_) {
      onFatalError();
      return false;
    }
    return true;
  }

  bool startConsume() {
    LOG_DEBUG("Start consume");
    if (consumer_ == nullptr) {
      return false;
    }
    const auto res = consumer_->subscribe(consumeTopics_);
    if (res != RdKafka::ERR_NO_ERROR) {
      onFatalError(RdKafka::err2str(res));
      return false;
    }
    return true;
  }

  bool startProduce() {
    LOG_DEBUG("Start produce");
    using namespace RdKafka;
    if (producer_ == nullptr) {
      return false;
    }
    for (auto &topic : produceTopics_) {
      UPtr<Topic> topicPtr{Topic::create(producer_.get(), topic, nullptr, error_)};
      if (!topicPtr) {
        onFatalError(std::format("Failed to create topic {}: {}", topic, error_));
        return false;
      }
      produceTopicMap_.insert(std::make_pair(topic, std::move(topicPtr)));
    }
    return true;
  }

  void pollConsume() {
    using namespace RdKafka;
    for (size_t i = 0; i < CONSUME_CHUNK && started_; ++i) {
      const UPtr<Message> msg{consumer_->consume(0)};
      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        ConsumeSerializerT::deserialize(static_cast<uint8_t *>(msg->payload()), msg->len(), bus_);
      } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
        LOG_ERROR("Failed to poll kafka messages: {}", static_cast<int32_t>(msg->err()));
        break;
      } else {
        break;
      }
    }
  }

  void schedulePoll() {
    if (!started_) {
      return;
    }
    timer_.expires_after(pollRate_);
    timer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ASIO_ERR_ABORTED) {
          onFatalError(code.message());
        }
        return;
      }
      pollConsume();
      producer_->poll(0);
      schedulePoll();
    });
  }

  void onFatalError(CRef<String> error = "") {
    if (!error.empty()) {
      error_ = error;
    }
    bus_.post(KafkaEvent{KafkaStatus::Error, error_});
  }

private:
  void printTopics() {
    std::stringstream ss;
    ss << "Kafka consume topics: ";
    for (auto &topic : consumeTopics_) {
      ss << topic << " ";
    }
    ss << std::endl << "Kafka produce topics: ";
    for (auto &topic : produceTopics_) {
      ss << topic << " ";
    }
    LOG_DEBUG_SYSTEM("{}", ss.str());
  }

  bool error() const { return !error_.empty(); }

private:
  BusT &bus_;

  const String broker_;
  const String consumerGroup_;
  const Microseconds pollRate_;

  const Vector<String> consumeTopics_;
  const Vector<String> produceTopics_;

  UPtr<RdKafka::Producer> producer_;
  UPtr<RdKafka::KafkaConsumer> consumer_;

  HashMap<String, UPtr<RdKafka::Topic>> produceTopicMap_;

  SteadyTimer timer_;

  bool started_{false};
  String error_;

  KafkaEventCallback eventCb_;
  KafkaDeliveryCallback deliveryCb_;
};
} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_KAFKAADAPTER_HPP
