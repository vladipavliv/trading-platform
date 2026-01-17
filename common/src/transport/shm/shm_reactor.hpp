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
 * @brief
 */
class ShmReactor {
  static constexpr size_t MAX_READERS = 4;

public:
  /**
   * @brief not a singletone, but a service locator
   */
  inline static Atomic<ShmReactor *> instance = nullptr;

  explicit ShmReactor(ErrorBus &&bus);
  ~ShmReactor();

  ShmReactor(const ShmReactor &) = delete;
  ShmReactor &operator=(const ShmReactor &) = delete;

  ShmReactor(ShmReactor &&) = delete;
  ShmReactor &operator=(ShmReactor &&) = delete;

  void run();
  void stop();

  void add(ShmReader *reader);
  bool running() const;

private:
  ShmReactor() = default;

  void loop();
  void cleanClosed();

private:
  std::vector<ShmReader *> readers_;
  AtomicBool running_{false};

  ErrorBus bus_;
  std::jthread thread_;
};
} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP