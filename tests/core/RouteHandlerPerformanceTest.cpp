#include "Helpers/RouteHandlerTestHelpers.cpp"
#include "RouteHandler.h"
#include "types/ParsedRoute.h"
#include <chrono>
#include <fmt/color.h>
#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace RouteParser;

namespace RouteHandlerTests
{
  class PerformanceTest : public ::testing::Test
  {
  protected:
    RouteHandler handler;

    void SetUp() override
    {
      // Any setup needed before each test
    }
  };

  TEST_F(PerformanceTest, BasicRouteWithSIDAndSTAR)
  {
    const auto startTime = std::chrono::steady_clock::now();

    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R", "ZSNJ", "VHHH");

    const auto endTime = std::chrono::steady_clock::now();
    const auto calculationDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
            .count();

    EXPECT_BASIC_ROUTE(parsedRoute);
    EXPECT_EQ(parsedRoute.rawRoute,
              "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R");
    EXPECT_EQ(parsedRoute.totalTokens, 7);

    EXPECT_LT(calculationDuration, 3);
  }

} // namespace RouteHandlerTests