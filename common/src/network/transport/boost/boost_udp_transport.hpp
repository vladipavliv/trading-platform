/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_BOOSTUDPTRANSPORT_HPP
#define HFT_COMMON_BOOSTUDPTRANSPORT_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>

#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/network_buffer.hpp"
#include "network/transport/boost/boost_network_types.hpp"
#include "primitive_types.hpp"

namespace hft {

class BoostUdpTransport {
public:
  explicit BoostUdpTransport(UdpSocket &&socket) : socket_{std::move(socket)} {}

  template <typename Callback>
  void asyncRx(ByteSpan buf, Callback &&clb) {
    socket_.async_receive(
        boost::asio::buffer(buf.data(), buf.size()),
        [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
          clb(ec ? IoResult::Error : IoResult::Ok, bytes);
        });
  }

  template <typename Callback>
  void asyncTx(ByteSpan buf, Callback &&clb) {
    socket_.async_send(
        boost::asio::buffer(buf.data(), buf.size()),
        [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
          clb(ec ? IoResult::Error : IoResult::Ok, bytes);
        });
  }

  void close() noexcept {
    BoostErrorCode ec;
    socket_.close(ec);
  }

private:
  UdpSocket socket_;
};

} // namespace hft

#endif // HFT_COMMON_BOOSTUDPTRANSPORT_HPP