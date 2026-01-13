/**
 * @author Vladimir Pavliv
 * @date 2025-12-24
 */

#ifndef HFT_COMMON_BUSRESTRICTOR_HPP
#define HFT_COMMON_BUSRESTRICTOR_HPP

#include <typeinfo>

#include "bus/busable.hpp"
#include "ptr_types.hpp"

namespace hft {

/**
 * @brief Routes only allowed types, blocks any other types
 */
template <typename BusT, typename... MessageTs>
struct BusRestrictor {
public:
  static_assert(Busable<BusT>, "BusT must satisfy the Busable concept");

  explicit BusRestrictor(BusT &bus) : bus_{bus} {}

  template <typename T>
  static constexpr bool isAllowed() {
    return (std::is_same_v<T, MessageTs> || ...);
  }

  template <typename T>
  void post(CRef<T> message) {
    if constexpr (isAllowed<T>()) {
      bus_.post(message);
    } else {
      LOG_ERROR("Type is not allowed {}", typeid(T).name());
    }
  }

private:
  BusT &bus_;
};
} // namespace hft

#endif // HFT_COMMON_BUSRESTRICTOR_HPP
