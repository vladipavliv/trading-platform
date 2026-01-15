/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_MEMORYUTILS_HPP
#define HFT_COMMON_MEMORYUTILS_HPP

#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

#include "logging.hpp"

namespace hft::utils {

/**
 * @brief Locks memory to prevent swapping
 */
inline void lockMemory(void *addr, size_t size) {
  if (mlock(addr, size) != 0) {
    throw std::system_error(errno, std::generic_category(), "mlock failed");
  }
}

/**
 * @brief "Warms up" memory by touching every page.
 */
inline void warmMemory(void *addr, size_t size, bool write = false) {
  volatile uint64_t *p = static_cast<volatile uint64_t *>(addr);
  size_t iterations = size / sizeof(uint64_t);

  for (size_t i = 0; i < iterations; i += 8) {
    p[i] = p[i];
  }
  if (write) {
    std::memset(addr, 0, size);
  }
}

/**
 * @brief Maps a shared memory file.
 */
inline void *mapSharedMemory(const std::string &name, size_t size, bool create) {
  const int flags = create ? (O_CREAT | O_RDWR | O_EXCL) : O_RDWR;
  const int fd = open(name.c_str(), flags, 0666);

  if (fd == -1) {
    throw std::system_error(errno, std::generic_category(), "Failed to open SHM: " + name);
  }

  if (create && ftruncate(fd, size) == -1) {
    close(fd);
    throw std::system_error(errno, std::generic_category(), "ftruncate failed");
  }

  void *addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (addr == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(), "mmap failed");
  }

  return addr;
}

static constexpr size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024;

constexpr size_t alignHuge(size_t size) {
  return (size + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);
}

template <typename T = void>
inline T *allocHuge(size_t size, bool lock = true) {
  const size_t alignedSize = alignHuge(size);

  void *ptr = mmap(nullptr, alignedSize, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);

  if (ptr == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(), "mmap HugePages failed");
  }

  if (lock && mlock(ptr, alignedSize) != 0) {
    munmap(ptr, alignedSize);
    throw std::system_error(errno, std::generic_category(), "mlock failed");
  }

  warmMemory(ptr, size, true);
  return static_cast<T *>(ptr);
}

inline void freeHuge(void *ptr, size_t size) {
  if (!ptr) {
    return;
  }

  const size_t aligned_size = (size + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);

  munlock(ptr, aligned_size);
  munmap(ptr, aligned_size);
}

} // namespace hft::utils

#endif // HFT_COMMON_MEMORYUTILS_HPP
