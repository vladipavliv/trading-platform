/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_COMMON_SHMREACTOR_HPP
#define HFT_COMMON_SHMREACTOR_HPP

#include <thread>
#include <vector>

#include "bus/system_bus.hpp"
#include "primitive_types.hpp"

namespace hft {

class ShmReader;

/**
 * @brief polls shm readers
 * currently only 1 reader is active so waiting for data is done on its futex
 * if more readers are needed utils::waitForAny could be used
 */
class ShmReactor {
public:
  /**
   * @brief service locator
   */
  inline static Atomic<ShmReactor *> instance = nullptr;

  explicit ShmReactor(const Config &cfg, std::stop_token token, ErrorBus &&bus);
  ~ShmReactor();

  ShmReactor(const ShmReactor &) = delete;
  ShmReactor(ShmReactor &&) = delete;
  ShmReactor &operator=(const ShmReactor &) = delete;
  ShmReactor &operator=(ShmReactor &&) = delete;

  void run(StdCallback onReadyClb = nullptr);
  bool running() const;
  void stop();
  void add(ShmReader *reader);

private:
  ShmReactor() = default;

  void loop();

private:
  const Config &config_;
  std::stop_token stopToken_;
  ErrorBus bus_;

  std::vector<ShmReader *> readers_;
  AtomicBool started_{false};

  std::jthread thread_;
};
} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP