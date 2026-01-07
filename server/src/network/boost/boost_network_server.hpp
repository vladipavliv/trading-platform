/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_SERVER_BOOSTNETWORKSERVER_HPP
#define HFT_SERVER_BOOSTNETWORKSERVER_HPP

#include <format>
#include <memory>

#include "commands/command.hpp"
#include "config/server_config.hpp"
#include "events.hpp"
#include "internal_error.hpp"
#include "network/transport/boost/boost_network_types.hpp"
#include "network/transport/boost/boost_tcp_transport.hpp"
#include "network/transport/boost/boost_udp_transport.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"

namespace hft::server {

/**
 * @brief Runs network io_context. Starts tcp acceptors and broadcast service
 * Redirects accepted tcp sockets to the gateway
 */
class BoostNetworkServer {
public:
  using StreamClb = std::function<void(BoostTcpTransport &&transport)>;
  using DatagramClb = std::function<void(BoostUdpTransport &&transport)>;

  explicit BoostNetworkServer(ServerBus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus}, upstreamAcceptor_{ioCtx_},
        downstreamAcceptor_{ioCtx_} {}

  ~BoostNetworkServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    if (running_) {
      LOG_ERROR("BoostNetworkServer is already running");
      return;
    }
    workerThread_ = std::jthread([this]() {
      try {
        running_ = true;
        utils::setThreadRealTime();
        if (ServerConfig::cfg.coreNetwork.has_value()) {
          const CoreId id = ServerConfig::cfg.coreNetwork.value();
          utils::pinThreadToCore(id);
          LOG_DEBUG("Network thread started on the core {}", id);
        } else {
          LOG_DEBUG("Network thread started");
        }
        ioCtx_.post([this]() {
          startUpstream();
          startDownstream();
          createDatagram();
        });
        ioCtx_.run();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        ioCtx_.stop();
        bus_.post(InternalError(StatusCode::Error, e.what()));
      }
    });
  }

  void stop() {
    if (!running_.exchange(false)) {
      return;
    }
    ioCtx_.stop();
    if (workerThread_.joinable()) {
      workerThread_.join();
    }
  }

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
      upstreamClb_(BoostTcpTransport{std::move(socket)});
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
      downstreamClb_(BoostTcpTransport{std::move(socket)});
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

      datagramClb_(BoostUdpTransport{std::move(socket)});
    } catch (const std::exception &ex) {
      LOG_ERROR_SYSTEM("Failed to create datagram transport: {}", ex.what());
    }
  }

private:
  IoCtx ioCtx_;
  IoCtxGuard guard_;

  ServerBus &bus_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  TcpAcceptor upstreamAcceptor_;
  TcpAcceptor downstreamAcceptor_;

  AtomicBool running_{false};
  std::jthread workerThread_;
};

} // namespace hft::server

#endif // HFT_SERVER_BOOSTNETWORKSERVER_HPP
