/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "template_types.hpp"
#include "types.hpp"

namespace hft {

struct BufferPool {
  static constexpr size_t MAX_MESSAGE_SIZE = 256;

  BufferPool(size_t size) : mBuffers(size / MAX_MESSAGE_SIZE + 1) {
    mMemPool = std::aligned_alloc(CACHE_LINE_SIZE, size);
    if (!mMemPool) {
      throw std::bad_alloc();
    }
    uint8_t *cursor = static_cast<uint8_t *>(mMemPool);
    for (int i = 0; i < size / MAX_MESSAGE_SIZE; ++i) {
      mBuffers.push(cursor);
      cursor += MAX_MESSAGE_SIZE;
    }
  }
  ~BufferPool() { std::free(mMemPool); }

  uint8_t *acquire() {
    uint8_t *buffer;
    while (!mBuffers.pop(buffer)) {
      spdlog::error("Buffer use limit reached");
      std::this_thread::yield();
    }
    return buffer;
  }

  void release(uint8_t *buffer) {
    while (!mBuffers.push(buffer)) {
      spdlog::error("Buffers capacity miscalculation");
      std::this_thread::yield();
    }
  }

private:
  boost::lockfree::queue<uint8_t *> mBuffers;
  void *mMemPool;
};

struct BufferGuard {
  BufferGuard(uint8_t *bufferPtr, BufferPool &bPool) : buffer{bufferPtr}, pool{bPool} {}
  ~BufferGuard() { pool.release(buffer); }

  uint8_t *buffer;
  BufferPool &pool;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
