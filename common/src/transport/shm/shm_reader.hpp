/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMREADER_HPP
#define HFT_COMMON_SHMREADER_HPP

#include "bus/system_bus.hpp"
#include "config/config.hpp"
#include "container_types.hpp"
#include "containers/sequenced_spsc.hpp"
#include "io_result.hpp"
#include "primitive_types.hpp"
#include "shm_ptr.hpp"
#include "shm_queue.hpp"
#include "utils/spin_wait.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

class ShmReactor;

class ShmReader {
  enum class State : uint8_t { Ready, Active, Closing, Closed };

public:
  enum class Result : uint8_t { Idle, Busy, Vanished };

  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  explicit ShmReader(CRef<String> name);
  ShmReader(ShmReader &&other);
  ~ShmReader();

  ShmReader &operator=(ShmReader &&other) = delete;
  ShmReader(const ShmReader &) = delete;

  void asyncRx(ByteSpan buf, RxHandler clb);
  void close();

  Result poll();

  void wait();
  void notify();

private:
  ShmReactor &init();
  void readLoop();

private:
  ShmUPtr<ShmQueue> shm_;
  ShmReactor &reactor_;

  ByteSpan buf_;
  RxHandler clb_;
  Atomic<State> state_{State::Ready};
};

} // namespace hft

#endif // HFT_COMMON_SHMREADER_HPP