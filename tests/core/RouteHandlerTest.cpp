#include "RouteHandler.h"
#include "types/ParsedRoute.h"
#include <algorithm>
#include <gtest/gtest.h>

using namespace RouteParser;

namespace RouteHandlerTests {
class RouteHandlerTest : public ::testing::Test {
protected:
  RouteHandler handler;

  void SetUp() override {
    // Any setup needed before each test
  }
};

void EXPECT_PARSE_ERROR_WITH_LEVEL(const ParsedRoute &parsedRoute,
                                   ParsingErrorLevel level,
                                   int expectedErrors) {
  const auto errors = parsedRoute.errors;
  const auto errorsWithLevel =
      std::count_if(errors.begin(), errors.end(), [level](const auto &error) {
        return error.level == level;
      });

  EXPECT_EQ(errorsWithLevel, expectedErrors);
}

void EXPECT_PARSE_ERROR_OF_TYPE(const ParsedRoute &parsedRoute,
                                ParsingErrorType type, int expectedErrors) {
  const auto errors = parsedRoute.errors;
  const auto errorsOfType =
      std::count_if(errors.begin(), errors.end(),
                    [type](const auto &error) { return error.type == type; });

  EXPECT_EQ(errorsOfType, expectedErrors);
}

void EXPECT_BASIC_ROUTE(ParsedRoute parsedRoute) {
  EXPECT_TRUE(parsedRoute.waypoints.empty()); // We don't have a database so
                                              // expecting no parsed waypoints

  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::INFO, 9);
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 0);
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                             7);
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_PROCEDURE,
                             2);

  EXPECT_EQ(parsedRoute.SID, "TES61X");
  EXPECT_EQ(parsedRoute.departureRunway, "06");

  EXPECT_EQ(parsedRoute.STAR, "ABBEY3A");
  EXPECT_EQ(parsedRoute.arrivalRunway, "07R");
}

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
      " TES61X/06 TESIG      A470 DOTMI  V512  :ABBEY ABBEY3A/07R", "ZSNJ",
      "VHHH");

  EXPECT_BASIC_ROUTE(parsedRoute);
  EXPECT_EQ(parsedRoute.totalTokens, 7);
}

TEST_F(RouteHandlerTest, UnknownWaypoint) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "KSFO SID1 INVALID_WPT STAR1 KLAX", "KSFO", "KLAX");

  EXPECT_FALSE(parsedRoute.errors.empty());
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                             3);
}

TEST_F(RouteHandlerTest, DepartureArrivalRunways) {
  auto parsedRoute = handler.GetParser()->ParseRawRoute(
      "BLUE DCT PAINT KLAX/24R", "KSFO", "KLAX");

  EXPECT_EQ(parsedRoute.departureRunway, "28L");
  EXPECT_EQ(parsedRoute.arrivalRunway, "24R");
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 0);
}

} // namespace RouteHandlerTests