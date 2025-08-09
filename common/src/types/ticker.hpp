/**
 * @author Vladimir Pavliv
 * @date 2025-04-016
 */

#ifndef HFT_COMMON_TICKER_HPP
#define HFT_COMMON_TICKER_HPP

#include <array>
#include <cstddef>
#include <cstring>
#include <string>

namespace hft {

constexpr size_t TICKER_SIZE = 4;
using Ticker = std::array<char, TICKER_SIZE>;

inline Ticker makeTicker(const std::string &str) {
  Ticker ticker{};
  std::memcpy(ticker.data(), str.data(), std::min(str.size(), TICKER_SIZE));
  return ticker;
}

struct TickerHash {
  std::size_t operator()(const Ticker &t) const {
    return std::hash<std::string_view>{}(std::string_view(t.data(), t.size()));
  }
};

} // namespace hft

#endif // HFT_COMMON_TICKER_HPP