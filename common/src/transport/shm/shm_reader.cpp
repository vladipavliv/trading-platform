/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#include "shm_reader.hpp"
#include "shm_reactor.hpp"

namespace hft {

ShmReader::ShmReader(CRef<String> name, bool createNew) : shm_{name, createNew} {}

ShmReader::ShmReader(ShmReader &&other)
    : shm_{std::move(other.shm_)}, buf_{std::move(other.buf_)}, clb_{std::move(other.clb_)} {}

void ShmReader::asyncRx(ByteSpan buf, RxHandler clb) {
  LOG_INFO_SYSTEM("asyncRx");
  buf_ = buf;
  clb_ = std::move(clb);
  ShmReactor::instance().add(this);
}

bool ShmReader::poll() {
  if (clb_ == nullptr) {
    LOG_ERROR_SYSTEM("Callback not set");
    return false;
  }
  const size_t bytes = shm_->queue.read(buf_.data(), buf_.size());
  if (bytes != 0) {
    clb_(IoResult::Ok, bytes);
    return true;
  }
  return false;
}

void ShmReader::wait() { shm_->wait(); }

void ShmReader::notify() { shm_->notify(); }

} // namespace hft
