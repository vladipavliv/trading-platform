/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_COMMON_SHMPTR_HPP
#define HFT_COMMON_SHMPTR_HPP

#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/memory_utils.hpp"

namespace hft {

template <typename T>
class ShmUPtr {
public:
  ShmUPtr(CRef<String> name, bool createNew)
      : name_{name}, createNew_{createNew}, alignedSize_(utils::alignHuge(sizeof(T))) {
    LOG_DEBUG_SYSTEM("ShmUPtr {} {}", name, createNew);
    if (createNew_) {
      unlink(name_.c_str());
    }
    void *addr = utils::mapSharedMemory(name_, alignedSize_, createNew_);

    utils::warmMemory(addr, alignedSize_, createNew_);
    utils::lockMemory(addr, alignedSize_);

    ptr_ = createNew_ ? (new (addr) T()) : static_cast<T *>(addr);
  }

  ShmUPtr(ShmUPtr &&other)
      : name_{other.name_}, createNew_{other.createNew_}, alignedSize_{other.alignedSize_},
        ptr_{other.ptr_} {
    other.ptr_ = nullptr;
  }

  ~ShmUPtr() {
    if (ptr_) {
      LOG_DEBUG_SYSTEM("~ShmUPtr {}", name_);
      munlock(ptr_, alignedSize_);
      munmap(ptr_, alignedSize_);
      if (createNew_) {
        unlink(name_.c_str());
      }
      ptr_ = nullptr;
    }
  }

  T *get() const { return ptr_; }
  T *operator->() const { return ptr_; }

private:
  const String name_;
  const bool createNew_;
  const size_t alignedSize_;

  T *ptr_{nullptr};
};
} // namespace hft

#endif // HFT_COMMON_SHMPTR_HPP