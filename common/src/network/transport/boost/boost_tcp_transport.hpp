/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_BOOSTTCPTRANSPORT_HPP
#define HFT_COMMON_BOOSTTCPTRANSPORT_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/network_buffer.hpp"
#include "types.hpp"

namespace hft {

class TcpTransport {
public:
  TcpTransport(TcpSocket &&socket) : socket_{std::move(socket)} {}

  template <typename Callback>
  void asyncRx(ByteSpan buf, Callback &&clb) {
    socket_.async_read_some(
        boost::asio::buffer(buf.data(), buf.size()),
        [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
          clb(ec ? IoResult::Error : IoResult::Ok, bytes);
        });
  }

  template <typename Callback>
  void asyncTx(ByteSpan buf, Callback &&clb) {
    boost::asio::async_write(
        socket_, boost::asio::buffer(buf.data(), buf.size()),
        [clb = std::forward<Callback>(clb)](BoostErrorCode ec, size_t bytes) mutable {
          clb(ec ? IoResult::Error : IoResult::Ok, bytes);
        });
  }

  void close() noexcept {
    boost::system::error_code ec;
    socket_.close(ec);
  }

private:
  TcpSocket socket_;
};

} // namespace hft

#endif // HFT_COMMON_BOOSTTCPTRANSPORT_HPP