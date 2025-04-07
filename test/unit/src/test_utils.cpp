/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#include <gtest/gtest.h>

#include "utils/string_utils.hpp"

namespace hft::tests {

TEST(UtilsTest, toStringTest) { EXPECT_EQ("42", utils::toString(42)); }

} // namespace hft::tests
