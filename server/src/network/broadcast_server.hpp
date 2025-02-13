/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_NETWORK_BROADCASTSERVER_HPP
#define HFT_SERVER_NETWORK_BROADCASTSERVER_HPP

#include <unordered_map>

#include "config/config.hpp"
#include "server_types.hpp"
#include "types/market_types.hpp"
#include "types/network_types.hpp"
#include "types/types.hpp"
#include "utils/utils.hpp"

namespace hft::server::network {

class BroadcastServer {
public:
  BroadcastServer(ServerSink &sink)
      : mSink{sink},
        mSocket(mSink.ioSink.ctx(), UdpEndpoint(Udp::v4(), Config::config().server.portUdp)),
        mEndpoint{boost::asio::ip::address::from_string(Config::config().server.url),
                  Config::config().server.portUdp} {}
  // TODO(do) io::address? Not v4?

  void start() { mSocket.set_option(boost::asio::socket_base::broadcast(true)); }
  void stop() {}

  template <typename Data>
  void send(Data &&data) {
    auto dataBuffer = Serializer::serialize<Data>(std::forward<Data>(data));
    auto msgPtr = utils::packMessage(std::move(dataBuffer));
    mSocket.async_send_to(
        boost::asio::buffer(msgPtr->data(), msgPtr->size()), mEndpoint,
        [this, msgPtr](BoostErrorRef ec, std::size_t size) { sendHandler(ec, size); });
  }

private:
  void sendHandler(BoostErrorRef ec, std::size_t size) {
    if (ec) {
      spdlog::error("Failed to send broadcast message: {}", ec.message());
    }
  }

  ServerSink &mSink;
  UdpSocket mSocket;
  UdpEndpoint mEndpoint;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_EGRESSSERVER_HPP