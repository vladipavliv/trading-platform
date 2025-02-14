/**
 * @file monitor.hpp
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONTROL_CENTER_HPP
#define HFT_COMMON_CONTROL_CENTER_HPP

#include <memory>
#include <spdlog/spdlog.h>

#include "console_input_parser.hpp"
#include "sink/command_sink.hpp"
#include "types/market_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

template <typename CommandType>
class ControlCenter {
public:
  using Command = CommandType;
  using Sink = CommandSink<Command>;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenter() : mConsoleParser{mSink} {}

  void run() { mConsoleParser.run(); }
  Sink &sink() { return mSink; }

private:
private:
  /*
  void generateData() {
    mTimer.expires_from_now(MilliSeconds(100));
    mTimer.async_wait([this](BoostErrorRef ec) {
      mDispatcher.network.template dispatch(PriceUpdate{generatePriceUpdate()});
    });
  }

  PriceUpdate generatePriceUpdate() {
    PriceUpdate update;
    for (int i = 0; i < 4; ++i) {
      update.ticker.data[i] = 'A' + (rand() % 26);
    }
    update.price = rand() % rand();
    return update;
  }
  */
  Sink mSink;
  ConsoleParser mConsoleParser;
};

} // namespace hft

#endif // HFT_COMMON_CONTROL_CENTER_HPP