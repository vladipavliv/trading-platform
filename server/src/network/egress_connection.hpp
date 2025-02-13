/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_NETWORK_EGRESSCONNECTION_HPP
#define HFT_SERVER_NETWORK_EGRESSCONNECTION_HPP

#include <memory>

#include "network_types.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::server::network {

class EgressConnection {
public:
  using UPtr = std::unique_ptr<EgressConnection>;

  EgressConnection(ServerSink &sink, TcpSocket socket)
      : mSink{sink}, mSocket{std::move(socket)}, mTraderId{utils::getTraderId(mSocket)} {}

  ~EgressConnection() { close(); }

  template <typename MessageType>
  void send(MessageType &&message) {
    auto buffer = Serializer::serialize(std::forward<MessageType>(message));
    auto dataPtr = utils::packMessage(std::move(buffer));
    boost::asio::async_write(
        mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
        [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
  }

  inline ObjectId objectId() const { return utils::getId(this); }
  void close() { mSocket.close(); }

private:
  void writeHandler(BoostErrorRef ec, size_t size) {
    if (ec) {
      spdlog::error("Failed to send message to {}, error: {}", mTraderId, ec.message());
      // No mercy
      // TODO(do) post disconnect event
    }
  }

private:
  ServerSink &mSink;
  TcpSocket mSocket;

  TraderId mTraderId;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_EGRESSCONNECTION_HPP