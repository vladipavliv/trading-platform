/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_COMMON_SHMREACTOR_HPP
#define HFT_COMMON_SHMREACTOR_HPP

#include <thread>

#include "primitive_types.hpp"

namespace hft {

class ShmReader;

/**
 * @brief For simplicity ipc layer needs to be stopped before closing transports
 */
class ShmReactor {
public:
  inline static ShmReactor &instance() {
    static ShmReactor reactor;
    return reactor;
  }

  void run();
  void stop();

  void add(ShmReader *reader);
  bool running() const;

private:
  ShmReactor() = default;
  void loop();

private:
  std::vector<ShmReader *> readers_;
  AtomicBool running_{false};

  std::jthread thread_;
};
} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP