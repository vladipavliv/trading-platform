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
#include "market_types.hpp"
#include "types.hpp"
#include "utils/json_utils.hpp"

namespace hft {

class KafkaCallback : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message &message) override {
    if (message.err()) {
      LOG_ERROR("Kafka delivery failed: {}", message.errstr());
    } else {
      LOG_DEBUG("Message delivered to the topic: {}", message.topic_name());
    }
  }
};

/**
 * @brief Kafka adapter
 * @todo Would require a dedicated DataBus with io_context. Producer is apparently thread-safe.
 */
class KafkaProducer {
public:
  KafkaProducer(SystemBus &bus, CRef<String> broker = "localhost:9092") : bus_{bus} {
    using namespace RdKafka;

    String error;
    conf_ = UPtr<Conf>(Conf::create(Conf::CONF_GLOBAL));

    if (conf_->set("dr_cb", &callback_, error) != Conf::CONF_OK) {
      LOG_ERROR("Failed to set delivery report callback: {}", error);
      throw std::runtime_error(error);
    }

    if (conf_->set("metadata.broker.list", broker, error) != Conf::CONF_OK) {
      LOG_ERROR("Failed to setup kafka brokers {}", error);
      throw std::runtime_error(error);
    }

    producer_ = UPtr<Producer>(Producer::create(conf_.get(), error));
    if (!producer_) {
      LOG_ERROR("Failed to create producer {}", error);
      throw std::runtime_error(error);
    }

    // Create Order topic
    orderTopic_ = UPtr<Topic>(Topic::create(producer_.get(), "order", nullptr, error));
    if (!orderTopic_) {
      LOG_ERROR("Failed to create Order topic {}", error);
      return;
    }

    // Create OrderStatus topic
    statusTopic_ = UPtr<Topic>(Topic::create(producer_.get(), "order-status", nullptr, error));
    if (!statusTopic_) {
      LOG_ERROR("Failed to create OrderStatus topic {}", error);
      return;
    }

    bus_.subscribe<Order>([this](CRef<Order> order) { produceOrder(order); });
    bus_.subscribe<OrderStatus>([this](CRef<OrderStatus> status) { produceOrderStatus(status); });
  };

private:
  void produceOrder(CRef<Order> order) {
    using namespace RdKafka;
    const auto msg = utils::toJson(order);
    const void *msgPtr = msg.data();

    const RdKafka::ErrorCode result =
        producer_->produce(orderTopic_.get(), Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                           const_cast<void *>(msgPtr), msg.size(), nullptr, nullptr);
    if (result != RdKafka::ERR_NO_ERROR) {
      LOG_ERROR("Failed to produce message {}", RdKafka::err2str(result));
    } else {
      LOG_DEBUG("Order sent to kafka");
    }
    producer_->poll(0);
  }

  void produceOrderStatus(CRef<OrderStatus> status) {
    using namespace RdKafka;
    const auto msg = utils::toJson(status);
    const void *msgPtr = msg.data();

    const RdKafka::ErrorCode result =
        producer_->produce(statusTopic_.get(), Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                           const_cast<void *>(msgPtr), msg.size(), nullptr, nullptr);
    if (result != RdKafka::ERR_NO_ERROR) {
      LOG_ERROR("Failed to produce message {}", RdKafka::err2str(result));
    } else {
      LOG_DEBUG("Order sent to kafka");
    }
    producer_->poll(0);
  }

private:
  SystemBus &bus_;

  UPtr<RdKafka::Producer> producer_;
  UPtr<RdKafka::Conf> conf_;

  UPtr<RdKafka::Topic> orderTopic_;
  UPtr<RdKafka::Topic> statusTopic_;

  KafkaCallback callback_;
};
} // namespace hft

#endif // HFT_COMMON_DB_KAFKAPRODUCER_HPP