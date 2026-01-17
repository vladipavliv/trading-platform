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

class ShmReader {
public:
  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  explicit ShmReader(CRef<String> name);
  ShmReader(ShmReader &&other);
  ~ShmReader() = default;

  ShmReader &operator=(ShmReader &&other) = delete;
  ShmReader(const ShmReader &) = delete;

  void asyncRx(ByteSpan buf, RxHandler clb);
  bool poll();
  void wait();
  void notify();

private:
  void readLoop();

private:
  ShmUPtr<ShmQueue> shm_;

  ByteSpan buf_;
  RxHandler clb_;
};

} // namespace hft

#endif // HFT_COMMON_SHMREADER_HPP