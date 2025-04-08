/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_DB_KAFKACALLBACK_HPP
#define HFT_COMMON_DB_KAFKACALLBACK_HPP

#include <librdkafka/rdkafkacpp.h>

#include "logging.hpp"

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

} // namespace hft

#endif // HFT_COMMON_DB_KAFKACALLBACK_HPP