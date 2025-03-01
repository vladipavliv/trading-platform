/**
 * @author Vladimir Pavliv
 * @date 2025-03-01
 */

#ifndef HFT_COMMON_OBJECTPOOL_HPP
#define HFT_COMMON_OBJECTPOOL_HPP

#include <boost/pool/singleton_pool.hpp>
#include <memory>

namespace hft {

template <typename ObjectType>
class PoolPtr {
  template <typename T>
  struct PoolTag {};

public:
  using PtrType = PoolPtr<ObjectType>;
  using PoolType = boost::singleton_pool<PoolTag<ObjectType>, sizeof(ObjectType)>;

  PoolPtr() = default;
  explicit PoolPtr(ObjectType *ptr) : mPtr{ptr} {}
  PoolPtr(PoolPtr &&other) noexcept : mPtr{std::exchange(other.mPtr, nullptr)} {}
  PoolPtr &operator=(PoolPtr &&other) noexcept {
    if (this != &other) {
      reset();
      mPtr = std::exchange(other.mPtr, nullptr);
    }
    return *this;
  }
  PoolPtr(const PoolPtr &) = delete;
  PoolPtr &operator=(const PoolPtr &) = delete;

  ~PoolPtr() { reset(); }

  ObjectType *operator->() { return mPtr; }
  const ObjectType *operator->() const { return mPtr; }
  ObjectType &operator*() { return *mPtr; }
  const ObjectType &operator*() const { return *mPtr; }
  explicit operator bool() const { return mPtr != nullptr; }

  void reset() {
    if (mPtr) {
      mPtr->~ObjectType();
      PoolType::free(mPtr);
      mPtr = nullptr;
    }
  }

  static PtrType create() {
    void *mem = PoolType::malloc();
    if (!mem) {
      throw std::bad_alloc();
    }
    return PtrType(new (mem) ObjectType());
  }

  friend void swap(PoolPtr &left, PoolPtr &right) noexcept { std::swap(left.mPtr, right.mPtr); }

private:
  ObjectType *mPtr{nullptr};
};

} // namespace hft

#endif // HFT_COMMON_OBJECTPOOL_HPP