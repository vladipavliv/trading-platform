/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_SERVER_BOOSTNETWORKSERVER_HPP
#define HFT_SERVER_BOOSTNETWORKSERVER_HPP

#include <format>
#include <memory>

#include "boost_types.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "internal_error.hpp"
#include "network/transport/boost/boost_tcp_transport.hpp"
#include "network/transport/boost/boost_udp_transport.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Runs network io_context. Starts tcp acceptors and broadcast service
 * Redirects accepted tcp sockets to the gateway
 */
class BoostNetworkServer {
public:
  using StreamTransport = TcpTransport;
  using DatagramTransport = UdpTransport;

  using StreamClb = std::function<void(StreamTransport &&transport)>;
  using DatagramClb = std::function<void(DatagramTransport &&transport)>;

  explicit BoostNetworkServer(ErrorBus &&bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{std::move(bus)}, upstreamAcceptor_{ioCtx_},
        downstreamAcceptor_{ioCtx_} {}

  ~BoostNetworkServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upStreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downStreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    workerThread_ = Thread([this]() {
      try {
        utils::setTheadRealTime();
        if (ServerConfig::cfg.coreNetwork.has_value()) {
          const CoreId id = ServerConfig::cfg.coreNetwork.value();
          utils::pinThreadToCore(id);
          LOG_DEBUG("Network thread started on the core {}", id);
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
    ioCtx_.post([this]() {
      startUpstream();
      startDownstream();
      createDatagram();
    });
  }

  void stop() { ioCtx_.stop(); }

  auto getHook() -> std::function<void(Callback &&clb)> {
    return [this](Callback &&clb) { ioCtx_.post(std::move(clb)); };
  }

private:
  void startUpstream() {
    const TcpEndpoint endpoint(Tcp::v4(), ServerConfig::cfg.portTcpUp);
    upstreamAcceptor_.open(endpoint.protocol());
    upstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    upstreamAcceptor_.bind(endpoint);
    upstreamAcceptor_.listen();
    acceptUpstream();
  }

  void acceptUpstream() {
    upstreamAcceptor_.async_accept([this](BoostErrorCode ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection {}", ec.message());
        return;
      }
      configureTcpSocket(socket);
      upStreamClb_(TcpTransport{std::move(socket)});
      acceptUpstream();
    });
  }

  void startDownstream() {
    const TcpEndpoint endpoint(Tcp::v4(), ServerConfig::cfg.portTcpDown);
    downstreamAcceptor_.open(endpoint.protocol());
    downstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    downstreamAcceptor_.bind(endpoint);
    downstreamAcceptor_.listen();
    acceptDownstream();
  }

  void acceptDownstream() {
    downstreamAcceptor_.async_accept([this](BoostErrorCode ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection: {}", ec.message());
        return;
      }
      configureTcpSocket(socket);
      downStreamClb_(TcpTransport{std::move(socket)});
      acceptDownstream();
    });
  }

  template <typename SocketType>
  void configureSocket(SocketType &socket) {
    using namespace boost::asio;
    socket.set_option(socket_base::send_buffer_size{1024 * 1024 * 4});
    socket.set_option(socket_base::receive_buffer_size{1024 * 1024 * 4});
  }

  void configureTcpSocket(TcpSocket &socket) {
    configureSocket(socket);
    socket.set_option(boost::asio::ip::tcp::no_delay(true));
  }

  void configureUdpSocket(UdpSocket &socket) {
    configureSocket(socket);
    socket.set_option(boost::asio::socket_base::reuse_address(true));
  }

  void createDatagram() {
    try {
      using namespace boost::asio;

      UdpSocket socket(ioCtx_, Udp::v4());

      socket.set_option(socket_base::broadcast{true});
      configureUdpSocket(socket);

      UdpEndpoint endpoint(ip::address_v4::broadcast(), ServerConfig::cfg.portUdp);
      socket.connect(endpoint);

      LOG_INFO_SYSTEM("UDP initialized {}:{}", endpoint.address().to_string(), endpoint.port());

      datagramClb_(DatagramTransport{std::move(socket)});
    } catch (const std::exception &ex) {
      LOG_ERROR_SYSTEM("Failed to create datagram transport: {}", ex.what());
    }
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  ErrorBus bus_;

  StreamClb upStreamClb_;
  StreamClb downStreamClb_;
  DatagramClb datagramClb_;

  TcpAcceptor upstreamAcceptor_;
  TcpAcceptor downstreamAcceptor_;

  Thread workerThread_;
};

} // namespace hft::server

#endif // HFT_SERVER_BOOSTNETWORKSERVER_HPP
