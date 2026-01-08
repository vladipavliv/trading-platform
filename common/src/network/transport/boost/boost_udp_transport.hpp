/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_BOOSTUDPTRANSPORT_HPP
#define HFT_COMMON_BOOSTUDPTRANSPORT_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>

#include "container_types.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/transport/boost/boost_network_types.hpp"
#include "primitive_types.hpp"

namespace hft {

class BoostUdpTransport {
public:
  explicit BoostUdpTransport(UdpSocket &&socket) : socket_{std::move(socket)} {}

  BoostUdpTransport(BoostUdpTransport &&) = default;
  BoostUdpTransport &operator=(BoostUdpTransport &&) = default;

  template <typename Callback>
  void asyncRx(ByteSpan buf, Callback &&clb) {
    using namespace boost::asio;
    auto handler = [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
      clb(toIoResult(ec), bytes);
    };
    static_assert(sizeof(handler) <= MAX_HANDLER_SIZE, "async handler is too large");
    socket_.async_receive(buffer(buf.data(), buf.size()), std::move(handler));
  }

  template <typename Callback>
  void asyncTx(ByteSpan buf, Callback &&clb) {
    using namespace boost::asio;
    auto handler = [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
      clb(toIoResult(ec), bytes);
    };
    static_assert(sizeof(handler) <= MAX_HANDLER_SIZE, "async handler is too large");
    socket_.async_send(buffer(buf.data(), buf.size()), std::move(handler));
  }

  void close() {
    BoostErrorCode ec;
    socket_.close(ec);
    if (ec) {
      LOG_ERROR("{}", ec.message());
    }
  }

private:
  UdpSocket socket_;
};

} // namespace hft

#endif // HFT_COMMON_BOOSTUDPTRANSPORT_HPP