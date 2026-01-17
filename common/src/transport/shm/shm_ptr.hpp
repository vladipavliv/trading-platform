/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_COMMON_SHMPTR_HPP
#define HFT_COMMON_SHMPTR_HPP

#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "shm_concepts.hpp"
#include "utils/memory_utils.hpp"

namespace hft {

template <RefCountedShm T>
class ShmUPtr {
public:
  explicit ShmUPtr(CRef<String> name) : name_{name}, alignedSize_(utils::alignHuge(sizeof(T))) {
    auto [ptr, own] = utils::mapSharedMemory(name_, alignedSize_);

    LOG_DEBUG_SYSTEM("ShmUPtr ctor {} {}", name_, own ? "created" : "opened");

    utils::warmMemory(ptr, alignedSize_, own);
    utils::lockMemory(ptr, alignedSize_);

    ptr_ = own ? (new (ptr) T()) : static_cast<T *>(ptr);
    ptr_->increment();
  }

  ShmUPtr(ShmUPtr &&other)
      : name_{other.name_}, alignedSize_{other.alignedSize_}, ptr_{other.ptr_} {
    LOG_DEBUG_SYSTEM("ShmUPtr mov {}", name_);
    other.ptr_ = nullptr;
  }

  ShmUPtr(const ShmUPtr &) = delete;
  ShmUPtr &operator=(const ShmUPtr &) = delete;

  ~ShmUPtr() {
    if (!ptr_) {
      return;
    }
    LOG_DEBUG_SYSTEM("~ShmUPtr dtor {}", name_);
    bool last = ptr_->decrement();

    munlock(ptr_, alignedSize_);
    munmap(ptr_, alignedSize_);
    if (last) {
      LOG_DEBUG_SYSTEM("Cleaning up {}", name_);
      unlink(name_.c_str());
    }
    ptr_ = nullptr;
  }

  T *get() const { return ptr_; }
  T *operator->() const { return ptr_; }

private:
  const String name_;
  const size_t alignedSize_;

  T *ptr_{nullptr};
};
} // namespace hft

#endif // HFT_COMMON_SHMPTR_HPP