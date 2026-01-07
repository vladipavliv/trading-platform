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
  volatile uint8_t *p = static_cast<volatile uint8_t *>(addr);
  for (size_t i = 0; i < size; i += 4096) {
    if (write) {
      p[i] = 0;
    } else {
      uint8_t dummy = p[i];
      (void)dummy;
    }
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

} // namespace hft::utils

#endif // HFT_COMMON_MEMORYUTILS_HPP
