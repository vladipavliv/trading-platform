/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_ADAPTERS_KAFKACALLBACKS_HPP
#define HFT_COMMON_ADAPTERS_KAFKACALLBACKS_HPP

#include <librdkafka/rdkafkacpp.h>

#include "logging.hpp"

namespace hft::adapters {

class KafkaEventCallback : public RdKafka::EventCb {
public:
  void event_cb(RdKafka::Event &event) override {
    // Map the kafka log levels to local log levels
    switch (event.type()) {
    case RdKafka::Event::Type::EVENT_ERROR:
      LOG_ERROR_SYSTEM("{}", RdKafka::err2str(event.err()));
      break;
    case RdKafka::Event::Type::EVENT_LOG:
      switch (event.severity()) {
      case RdKafka::Event::Severity::EVENT_SEVERITY_EMERG:
      case RdKafka::Event::Severity::EVENT_SEVERITY_ALERT:
      case RdKafka::Event::Severity::EVENT_SEVERITY_CRITICAL:
        LOG_ERROR_SYSTEM("{}", event.str());
        break;
      case RdKafka::Event::Severity::EVENT_SEVERITY_ERROR:
        LOG_ERROR_SYSTEM("{}", event.str());
        break;
      case RdKafka::Event::Severity::EVENT_SEVERITY_WARNING:
      case RdKafka::Event::Severity::EVENT_SEVERITY_NOTICE:
        LOG_WARN_SYSTEM("{}", event.str());
        break;
      case RdKafka::Event::Severity::EVENT_SEVERITY_INFO:
        LOG_INFO_SYSTEM("{}", event.str());
        break;
      case RdKafka::Event::Severity::EVENT_SEVERITY_DEBUG:
        LOG_DEBUG_SYSTEM("{}", event.str());
        break;
      default:
        break;
      }
      break;
    case RdKafka::Event::Type::EVENT_STATS:
    case RdKafka::Event::Type::EVENT_THROTTLE:
    default:
      break;
    }
  }
};

class KafkaDeliveryCallback : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message &message) override {
    if (message.err() != RdKafka::ERR_NO_ERROR) {
      LOG_ERROR("Kafka delivery failed: {}", message.errstr());
    }
  }
};

} // namespace hft::adapters

#endif // HFT_COMMON_ADAPTERS_KAFKACALLBACKS_HPP