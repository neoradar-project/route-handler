#include "RouteHandler.h"
#include "Helpers/RouteHandlerTestHelpers.cpp"
#include "types/ParsedRoute.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <optional>

using namespace RouteParser;

namespace RouteHandlerTests {
class RouteHandlerTest : public ::testing::Test {
protected:
  RouteHandler handler;

  void SetUp() override {
    // Any setup needed before each test
  }
};

TEST_F(RouteHandlerTest, EmptyRoute) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute("", "KSFO", "KLAX");

  EXPECT_TRUE(parsedRoute.waypoints.empty());
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::ROUTE_EMPTY, 1);

  EXPECT_EQ(parsedRoute.errors[0].type, ParsingErrorType::ROUTE_EMPTY);
  EXPECT_EQ(parsedRoute.totalTokens, 0);
}

TEST_F(RouteHandlerTest, BasicRouteWithSIDAndSTAR) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R", "ZSNJ", "VHHH");

  EXPECT_BASIC_ROUTE(parsedRoute);
  EXPECT_EQ(parsedRoute.rawRoute,
            "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R");
  EXPECT_EQ(parsedRoute.totalTokens, 7);
}

TEST_F(RouteHandlerTest, RouteWithSpacesAndColons) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      " TES61X/06 TESIG      A470 DOTMI  V512 : :ABBEY ABBEY3A/07R  ", "ZSNJ",
      "VHHH");

  EXPECT_BASIC_ROUTE(parsedRoute);
  EXPECT_EQ(parsedRoute.totalTokens, 7);
}

TEST_F(RouteHandlerTest, UnknownWaypoint) {
  // TODO: add database to check against actual invalid waypoints
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "KSFO SID1 INVALID_WPT STAR1 KLAX", "KSFO", "KLAX");

  EXPECT_FALSE(parsedRoute.errors.empty());
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                             3);
}

TEST_F(RouteHandlerTest, DepartureArrivalRunways) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "KSFO/28L BLUE DCT PAINT KLAX/24R ", "KSFO", "KLAX");

  EXPECT_EQ(parsedRoute.departureRunway, "28L");
  EXPECT_EQ(parsedRoute.arrivalRunway, "24R");
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 0);
  EXPECT_EQ(parsedRoute.totalTokens, 5);
}

TEST_F(RouteHandlerTest, DepartureArrivalRunwaysWithMismatchingICAO) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "KSFO/28L BLUE DCT PAINT KLAX/24R ", "RJTT", "LFPO");

  EXPECT_EQ(parsedRoute.departureRunway, std::nullopt);
  EXPECT_EQ(parsedRoute.arrivalRunway, std::nullopt);
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 2);
  EXPECT_EQ(parsedRoute.totalTokens, 5);
}

} // namespace RouteHandlerTests