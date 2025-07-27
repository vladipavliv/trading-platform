/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include <concepts>

#include "domain_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"

namespace hft {

namespace detail {
struct ProbeType {};
} // namespace detail

/**
 * @brief Defines a concept for a bus with templated ::post
 * There are no better ways that I'm aware of to do it, it would have been better if
 * ProbeType could be defined inside the concept guaraneeing that the only way ::post works
 * with this type - is if it has templated ::post. But this way works too. Beter then nothing
 */
template <typename Consumer>
concept Busable = requires(Consumer consumer) {
  {
    consumer.template post<detail::ProbeType>(std::declval<detail::ProbeType>())
  } -> std::same_as<void>;
};

/**
 * @brief Holds different types of buses, Routes messages to a proper one
 * @todo As of now all bus subscribers are long living objects, so unsub mechanism is not needed
 */
template <typename... MarketTypes>
struct BusHolder {
  using MarketBus = MessageBus<MarketTypes...>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline IoCtx &systemCtx() { return systemBus.ioCtx; }

  template <typename MessageType>
    requires(MarketBus::template RoutedType<MessageType>)
  inline void post(CRef<MessageType> message) {
    marketBus.template post<MessageType>(message);
  }

  template <typename MessageType>
    requires(!MarketBus::template RoutedType<MessageType>)
  inline void post(CRef<MessageType> message) {
    systemBus.template post<MessageType>(message);
  }

  void run() { systemBus.run(); }

  void stop() { systemBus.stop(); }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
