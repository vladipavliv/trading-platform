/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_CLIENT_BOOSTNETWORKCLIENT_HPP
#define HFT_CLIENT_BOOSTNETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <vector>

#include "boost_types.hpp"
#include "client_events.hpp"
#include "client_types.hpp"
#include "config/client_config.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/transport/boost/boost_tcp_transport.hpp"
#include "network/transport/boost/boost_udp_transport.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief
 */
class BoostNetworkClient {
public:
  using StreamTransport = TcpTransport;
  using DatagramTransport = UdpTransport;

  using StreamClb = std::function<void(StreamTransport &&)>;
  using DatagramClb = std::function<void(DatagramTransport &&)>;

  explicit BoostNetworkClient(ClientBus &bus)
      : bus_{bus}, guard_{MakeGuard(ioCtx_.get_executor())} {}

  ~BoostNetworkClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upStreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downStreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    workerThread_ = Thread([this]() {
      try {
        utils::setTheadRealTime();
        if (ClientConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ClientConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_DEBUG("Network thread started on the core {}", coreId);
        } else {
          LOG_DEBUG("Network thread started");
        }
        ioCtx_.run();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        ioCtx_.stop();
        bus_.post(InternalError(StatusCode::Error, e.what()));
      }
    });
  }

  void stop() { ioCtx_.stop(); }

  void connect() {
    ioCtx_.post([this]() {
      LOG_DEBUG("Connecting to the server");
      asyncConnect(ClientConfig::cfg.portTcpUp, upStreamClb_);
      asyncConnect(ClientConfig::cfg.portTcpDown, downStreamClb_);

      createPrices();
    });
  }

private:
  void asyncConnect(uint16_t port, const StreamClb &callback) {
    auto socket = std::make_shared<TcpSocket>(ioCtx_);
    TcpEndpoint endpoint(boost::asio::ip::make_address(ClientConfig::cfg.url), port);

    socket->async_connect(endpoint, [this, socket, callback, port](BoostErrorCode ec) {
      if (!ec) [[likely]] {
        configureTcpSocket(*socket);
        callback(StreamTransport{std::move(*socket)});
        LOG_INFO_SYSTEM("TCP connection established on port {}", port);
      } else {
        LOG_ERROR_SYSTEM("Connection failed on port {}: {}", port, ec.message());
      }
    });
  }

  void createPrices() {
    try {
      UdpSocket socket(ioCtx_, Udp::v4());
      socket.set_option(boost::asio::socket_base::reuse_address{true});

      configureSocket(socket);

      UdpEndpoint listenEndpoint(Udp::v4(), ClientConfig::cfg.portUdp);
      socket.bind(listenEndpoint);

      datagramClb_(DatagramTransport{std::move(socket)});
      LOG_INFO_SYSTEM("UDP listener bound to port {}", ClientConfig::cfg.portUdp);
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Failed to setup UDP pricing: {}", e.what());
    }
  }

  template <typename SocketType>
  void configureSocket(SocketType &socket) {
    using namespace boost::asio;
    socket.set_option(socket_base::send_buffer_size{1024 * 1024 * 4});
    socket.set_option(socket_base::receive_buffer_size{1024 * 1024 * 4});

    if constexpr (std::is_same_v<std::decay_t<SocketType>, TcpSocket>) {
      socket.set_option(ip::tcp::no_delay(true));
    }
  }

  void configureTcpSocket(TcpSocket &socket) {
    using namespace boost::asio;
    configureSocket(socket);
    socket.set_option(ip::tcp::no_delay(true));
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  ClientBus &bus_;

  StreamClb upStreamClb_;
  StreamClb downStreamClb_;
  DatagramClb datagramClb_;

  Thread workerThread_;
};

} // namespace hft::client

#endif // HFT_CLIENT_NETWORKCLIENT_HPP
