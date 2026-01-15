/**
 * @author Vladimir Pavliv
 * @date 2026-01-11
 */

#ifndef HFT_COMMON_SHM_HPP
#define HFT_COMMON_SHM_HPP

#include "config/config.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "shm_layout.hpp"
#include "utils/memory_utils.hpp"

namespace hft {

class ShmManager {
public:
  static void initialize(bool create) {
    if (layout_ != nullptr) {
      LOG_ERROR_SYSTEM("Shared memory is already initialized");
      return;
    }
    name_ = Config::get<String>("shm.shm_name");
    size_ = Config::get<size_t>("shm.shm_size");
    create_ = create;

    if (create) {
      unlink(name_.c_str());
    }

    void *addr = utils::mapSharedMemory(name_, size_, create);

    utils::warmMemory(addr, size_, create);
    utils::lockMemory(addr, size_);

    if (create_) {
      layout_ = new (addr) ShmLayout();
    } else {
      layout_ = static_cast<ShmLayout *>(addr);
    }
  }

  static void deinitialize() {
    if (layout_) {
      munmap(layout_, size_);
      if (create_) {
        unlink(name_.c_str());
        layout_ = nullptr;
      }
    }
  }

  static ShmLayout &layout() { return *layout_; }

private:
  inline static String name_;
  inline static size_t size_;
  inline static bool create_;

  inline static ShmLayout *layout_{nullptr};
};
} // namespace hft

#endif // HFT_COMMON_SHM_HPP