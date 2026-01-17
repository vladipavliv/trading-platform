/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#include "shm_reader.hpp"
#include "shm_reactor.hpp"

namespace hft {

ShmReader::ShmReader(CRef<String> name) : reactor_{init()}, shm_{name} {
  LOG_INFO_SYSTEM("ShmReader ctor");
}

ShmReader::~ShmReader() {
  auto state = state_.load(std::memory_order_acquire);

  SpinWait waiter{SPIN_RETRIES_YIELD};
  while (state != State::Closed && reactor_.running()) {
    if (state == State::Ready || state == State::Active) {
      state_.store(State::Closing);
      shm_->notify();
    }
    state = state_.load(std::memory_order_acquire);
    if (!++waiter) {
      LOG_ERROR_SYSTEM("Failed to properly close ShmReader");
      break;
    }
  }
}

ShmReactor &ShmReader::init() {
  ShmReactor *r = ShmReactor::instance.load(std::memory_order_acquire);
  if (r == nullptr) {
    throw std::runtime_error("ShmReactor is not initialized");
  }
  return *r;
}

ShmReader::ShmReader(ShmReader &&other)
    : shm_{std::move(other.shm_)}, reactor_{other.reactor_}, buf_{std::move(other.buf_)},
      clb_{std::move(other.clb_)}, state_{other.state_.load()} {
  other.state_ = State::Closed;
}

void ShmReader::asyncRx(ByteSpan buf, RxHandler clb) {
  if (state_ != State::Ready) {
    LOG_ERROR_SYSTEM("Invalid state in ShmReader");
    return;
  }
  LOG_INFO_SYSTEM("asyncRx");
  buf_ = buf;
  clb_ = std::move(clb);

  state_.store(State::Active, std::memory_order_release);
  reactor_.add(this);
}

ShmReader::Result ShmReader::poll() {
  switch (state_.load(std::memory_order_acquire)) {
  case State::Ready:
    return Result::Idle;
  case State::Active: {
    const size_t bytes = shm_->queue.read(buf_.data(), buf_.size());
    if (bytes != 0) {
      clb_(IoResult::Ok, bytes);
      return Result::Busy;
    }
    return Result::Idle;
  }
  case State::Closing:
    state_.store(State::Closed, std::memory_order_release);
    return Result::Vanished;
  default:
    return Result::Vanished;
  }
}

void ShmReader::close() {
  auto state = state_.load(std::memory_order_acquire);
  if (state == State::Ready || state == State::Active) {
    state_.store(State::Closing);
    shm_->notify();
  }
}

void ShmReader::wait() { shm_->wait(); }

void ShmReader::notify() { shm_->notify(); }

} // namespace hft
