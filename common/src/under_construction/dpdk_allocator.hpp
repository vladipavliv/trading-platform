/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_DPDKALLOCATOR_HPP
#define HFT_COMMON_DPDKALLOCATOR_HPP

#include <cstring>
#include <iostream>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memory.h>
#include <rte_mempool.h>

namespace hft {

class DpdkMempool {
public:
  static void init(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
      rte_panic("Failed to start eel");
    }
  }

  DpdkMempool(const char *name, unsigned n, unsigned elt_size, unsigned cache_size, int socket_id) {
    mPool =
        rte_mempool_create(name, n, elt_size, cache_size, sizeof(struct rte_pktmbuf_pool_private),
                           rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, socket_id, 0);
    if (mPool == nullptr) {
      rte_panic("Failed to create memory pool");
    }
  }

  ~DpdkMempool() {
    if (mPool) {
      rte_mempool_free(mPool);
    }
  }

  rte_mempool &getPool() { return *mPool; }

  // Allocate an element from the memory pool
  void *alloc() {
    void *obj;
    if (rte_mempool_get(mPool, &obj) < 0) {
      rte_panic("Dynamic memory allocation failed");
    }
    return obj;
  }

  // Free an element back to the memory pool
  void free(void *obj) { rte_mempool_put(mPool, obj); }

private:
  rte_mempool *mPool;
};

// Custom allocator for std::vector
template <typename Type>
class DpdkAllocator {
public:
  using ValueType = Type;

  explicit DpdkAllocator(DpdkMempool &pool) : mMempool(pool) {}

  Type *allocate(std::size_t n) {
    void *ptr = mMempool.alloc();
    if (!ptr) {
      rte_panic("Failed to allocate memory");
    }
    return static_cast<Type *>(ptr);
  }

  void deallocate(Type *ptr, std::size_t n) { mMempool.free(ptr); }
  bool operator==(const DpdkAllocator &other) const {
    return mMempool.getPool() == other.mMempool.getPool();
  }
  bool operator!=(const DpdkAllocator &other) const { return !(*this == other); }

private:
  DpdkMempool &mMempool;
};

} // namespace hft

#endif // HFT_COMMON_DPDKALLOCATOR_HPP