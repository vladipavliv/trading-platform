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

#ifdef CICD
static constexpr int ANON_FLAGS = MAP_PRIVATE | MAP_ANONYMOUS;
#else
static constexpr int ANON_FLAGS = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE;
static constexpr int SHM_FLAGS = MAP_SHARED | MAP_HUGETLB | MAP_POPULATE;
#endif

static constexpr size_t HUGE_PAGE_SIZE = 2 * 1024 * 1024;

constexpr size_t alignHuge(size_t size) {
  return (size + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);
}

inline void lockMemory(void *addr, size_t size) {
  if (mlock(addr, size) != 0) {
    throw std::system_error(errno, std::generic_category(), "mlock failed");
  }
}

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

struct ShmRes {
  void *ptr = nullptr;
  bool own = false;
};

inline ShmRes mapAnonymousShm(size_t size) {
  size = alignHuge(size);
  void *addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, ANON_FLAGS, -1, 0);
  if (addr == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(), "mmap anonymous failed");
  }
  return {addr, true};
}

inline ShmRes mapFileHuge(const std::string &path, size_t size) {
  size = alignHuge(size);

  struct stat st;
  bool existed = (stat(path.c_str(), &st) == 0);

  int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category(), "open file failed");
  }

  if (!existed && ftruncate(fd, size) == -1) {
    close(fd);
    throw std::system_error(errno, std::generic_category(), "ftruncate file failed");
  }

  void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, SHM_FLAGS, fd, 0);
  close(fd);
  if (ptr == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(), "mmap file failed");
  }

  return {ptr, !existed};
}

inline ShmRes mapSharedMemory(const std::string &name, size_t size) {
#ifdef CICD
  return mapAnonymousShm(size);
#else
  return mapFileHuge(name, size);
#endif
}

template <typename T = void>
inline T *allocHuge(size_t size, bool lock = true) {
  size = alignHuge(size);

  void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, ANON_FLAGS, -1, 0);

#ifndef CICD
  if (ptr == MAP_FAILED && errno == ENOMEM) {
    LOG_WARN("HugePages allocation failed, falling back to standard pages");
    int fallback_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, fallback_flags, -1, 0);
  }
#endif

  if (ptr == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category(), "mmap failed");
  }

  if (lock && mlock(ptr, size) != 0) {
    LOG_WARN("mlock failed (likely CI limit), continuing without memory lock");
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
