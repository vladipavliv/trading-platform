/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORK_INGRESSCONNECTION_HPP
#define HFT_SERVER_NETWORK_INGRESSCONNECTION_HPP

#include <memory>

#include "market_types.hpp"
#include "network_types.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server::network {

class IngressConnection {
public:
  using UPtr = std::unique_ptr<IngressConnection>;

  IngressConnection(ServerSink &sink, TcpSocket socket)
      : mSink{sink}, mSocket{std::move(socket)}, mTraderId{utils::getTraderId(mSocket)} {}

  ~IngressConnection() { close(); }

  void open() { doRead(true, sizeof(MessageSize)); }
  void close() { mSocket.close(); }
  inline ObjectId objectId() const { return utils::getId(this); }

private:
  void doRead(bool header, MessageSize size) {
    boost::asio::async_read(
        mSocket, boost::asio::buffer(mBuffer, size),
        [this, header](BoostErrorRef ec, size_t readSize) { readHandler(ec, readSize, header); });
  }

  void readHandler(BoostErrorRef ec, size_t readSize, bool header) {
    if (!mSocket.is_open()) {
      spdlog::error("Socket closed unexpectedly");
      return;
    }
    if (ec) {
      spdlog::error("Failed to read request: {}", ec.message());
      // No mercy
      // TODO(do) mDispatcher.network.dispatch(DisconnectEvent{objectId()});
      return;
    }
    // TODO(self): Take care of readSize for bigger messages
    if (header) {
      if (readSize != sizeof(MessageSize)) {
        spdlog::error("Failed to read header: wrong size");
        return;
      }
      MessageSize bodySize = 0;
      std::memcpy(&bodySize, mBuffer.data(), sizeof(MessageSize));
      bodySize = ntohs(bodySize);

      mBuffer.fill(0);
      spdlog::debug("Reading body, size: {}", bodySize);
      doRead(false, bodySize);
    } else {
      auto result = Serializer::deserialize<Order>(mBuffer, readSize);

      mBuffer.fill(0);
      if (result.ok()) {
        Order order = result.extract();
        // TODO(do) order.traderId = utils::getTraderId(mSocket);
        mSink.dataSink.post(std::move(order));
      } else {
        spdlog::error("Failed to parse Order");
      }
      doRead(true, sizeof(MessageSize));
    }
  }

private:
  ServerSink &mSink;
  TcpSocket mSocket;

  Buffer mBuffer{};
  TraderId mTraderId;
};
} // namespace hft::server::network

#endif // HFT_SERVER_NETWORK_INGRESSCONNECTION_HPP