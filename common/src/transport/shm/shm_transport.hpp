/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_SHMTRANSPORT_HPP
#define HFT_COMMON_SHMTRANSPORT_HPP

#include "bus/system_bus.hpp"
#include "functional_types.hpp"
#include "shm_queue.hpp"
#include "shm_reactor.hpp"
#include "shm_reader.hpp"
#include "shm_writer.hpp"
#include "utils/handler.hpp"

namespace hft {

class ShmTransport {
public:
  enum class Type : uint8_t { None, Reader, Writer };

  static ShmTransport makeReader(CRef<String> name) {
    ShmTransport t(Type::Reader);
    t.reader_.emplace(name);
    return t;
  }

  static ShmTransport makeWriter(CRef<String> name) {
    ShmTransport t(Type::Writer);
    t.writer_.emplace(name);
    return t;
  }

  ShmTransport(ShmTransport &&other) noexcept = default;
  ShmTransport &operator=(ShmTransport &&other) noexcept = delete;

  ~ShmTransport() = default;

  Type type() const noexcept { return type_; }

  IoResult syncRx(ByteSpan buf) {
    LOG_DEBUG("ShmTransport syncRx");
    if (type_ == Type::Reader) {
      return reader_->syncRx(buf);
    }
    LOG_ERROR_SYSTEM("Unable to read from shm: wrong transport type");
    return {0, IoStatus::Error};
  }

  void asyncRx(ByteSpan buf, CRefHandler<IoResult> &&clb) {
    LOG_DEBUG("ShmTransport asyncRx");
    if (type_ == Type::Reader) {
      reader_->asyncRx(buf, std::move(clb));
    } else {
      LOG_ERROR_SYSTEM("Unable to read from shm: wrong transport type");
    }
  }

  IoResult syncTx(CByteSpan buffer) {
    LOG_DEBUG("ShmTransport syncTx");
    if (type_ == Type::Writer) {
      return writer_->syncTx(buffer);
    }
    LOG_ERROR_SYSTEM("Unable to write to shm: wrong transport type");
    return {0, IoStatus::Error};
  }

  void asyncTx(CByteSpan buffer, CRefHandler<IoResult> &&clb) {
    LOG_DEBUG("ShmTransport asyncTx");
    if (type_ == Type::Writer) {
      writer_->asyncTx(buffer, std::move(clb));
    } else {
      LOG_ERROR_SYSTEM("Unable to write to shm: wrong transport type");
    }
  }

  void close() {
    if (reader_.has_value()) {
      reader_->close();
    }
    if (writer_.has_value()) {
      writer_->close();
    }
  }

private:
  ShmTransport() : type_(Type::None) {}
  explicit ShmTransport(Type t) : type_(t) {}

private:
  Type type_;

  std::optional<ShmReader> reader_;
  std::optional<ShmWriter> writer_;
};

} // namespace hft

#endif // HFT_COMMON_SHMTRANSPORT_HPP