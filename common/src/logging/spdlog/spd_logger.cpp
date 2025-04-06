/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#include "spd_logger.hpp"

namespace hft {

SpdLogger::SPtrSpdLogger SpdLogger::consoleLogger{};
SpdLogger::SPtrSpdLogger SpdLogger::fileLogger{};

} // namespace hft