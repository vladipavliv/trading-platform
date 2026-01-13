/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_SHMTRANSPORT_HPP
#define HFT_COMMON_SHMTRANSPORT_HPP

#include "bus/system_bus.hpp"
#include "shm_queue.hpp"
#include "shm_reader.hpp"
#include "shm_writer.hpp"

namespace hft {

class ShmTransport {
public:
  using RxHandler = std::function<void(IoResult, size_t)>;

  enum class Type : uint8_t { Reader, Writer };

  static ShmTransport makeReader(ShmQueue &q, ErrorBus bus) {
    ShmTransport t(Type::Reader);
    new (&t.reader_) ShmReader(q, std::move(bus));
    return t;
  }

  static ShmTransport makeWriter(ShmQueue &q) {
    ShmTransport t(Type::Writer);
    new (&t.writer_) ShmWriter(q);
    return t;
  }

  ShmTransport(ShmTransport &&other) noexcept : type_(other.type_) {
    if (type_ == Type::Reader) {
      new (&reader_) ShmReader(std::move(other.reader_));
    } else {
      new (&writer_) ShmWriter(std::move(other.writer_));
    }
  }

  ShmTransport &operator=(ShmTransport &&other) noexcept = delete;

  ~ShmTransport() {
    close();
    destroy();
  }

  Type type() const noexcept { return type_; }

  void asyncRx(ByteSpan buf, RxHandler clb) {
    LOG_DEBUG("ShmTransport asyncRx");
    if (type_ == Type::Reader) {
      reader_.asyncRx(buf, std::move(clb));
    }
  }

  void asyncTx(CByteSpan buffer, RxHandler clb, uint32_t retry = SPIN_RETRIES_HOT) {
    LOG_DEBUG("ShmTransport asyncTx");
    if (type_ == Type::Writer) {
      writer_.asyncTx(buffer, std::move(clb), retry);
    }
  }

  void close() {
    if (type_ == Type::Writer) {
      writer_.close();
    } else {
      reader_.close();
    }
  }

private:
  explicit ShmTransport(Type t) : type_(t) {}

  void destroy() noexcept {
    if (type_ == Type::Reader) {
      reader_.~ShmReader();
    } else {
      writer_.~ShmWriter();
    }
  }

private:
  Type type_;

  union {
    ShmReader reader_;
    ShmWriter writer_;
  };
};

} // namespace hft

#endif // HFT_COMMON_SHMTRANSPORT_HPP