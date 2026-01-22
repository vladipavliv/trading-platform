/**
 * @author Vladimir Pavliv
 * @date 2026-01-22
 */

#ifndef HFT_COMMON_APPCONTEXT_HPP
#define HFT_COMMON_APPCONTEXT_HPP

#include <stop_token>

namespace hft {
template <typename BusT, typename ConfigT>
struct AppContext {
  BusT &bus;
  const ConfigT &config;
  std::stop_token stopToken;
};
} // namespace hft

#endif // HFT_COMMON_APPCONTEXT_HPP