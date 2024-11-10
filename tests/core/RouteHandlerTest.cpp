#include "RouteHandler.h"
#include "Data/SampleNavdata.cpp"
#include "Helpers/RouteHandlerTestHelpers.cpp"
#include "types/ParsedRoute.h"
#include "types/Waypoint.h"
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
    handler = RouteHandler();
    // Any setup needed before each test
    const std::function<void(const char *, const char *)> logFunc =
        [](const char *msg, const char *level) {
          // fmt::print(fg(fmt::color::yellow), "[{}] {}\n", level, msg);
          // Ignore log messages in tests
        };
    handler.Bootstrap(logFunc, Data::SmallWaypointsList, {});
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
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "KSFO BLUE INVALID_WPT TESIG KLAX", "KSFO", "KLAX");

  EXPECT_FALSE(parsedRoute.errors.empty());
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                             1);
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

TEST_F(RouteHandlerTest, ChangeOfFlightRule) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "VFR BLUE DCT IFR PAINT", "KSFO", "KLAX");

  ASSERT_EQ(parsedRoute.waypoints.size(), 2);
  EXPECT_EQ(parsedRoute.waypoints[0].GetFlightRule(), FlightRule::VFR);
  EXPECT_EQ(parsedRoute.waypoints[1].GetFlightRule(), FlightRule::IFR);
}

TEST_F(RouteHandlerTest, LatLonInRoute) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "BLUE DCT PAINT DCT 5220N03305E", "KSFO", "KLAX");

  ASSERT_EQ(parsedRoute.waypoints.size(), 3);
  EXPECT_EQ(parsedRoute.errors.size(), 0);
  EXPECT_EQ(parsedRoute.waypoints[2].getType(), WaypointType::LATLON);
  EXPECT_EQ(parsedRoute.waypoints[2].getIdentifier(), "5220N03305E");
  EXPECT_EQ(parsedRoute.waypoints[2].getPosition().latitude().degrees(),
            52.333333333333336);
  EXPECT_EQ(parsedRoute.waypoints[2].getPosition().longitude().degrees(),
            33.083333333333336);
}

} // namespace RouteHandlerTests