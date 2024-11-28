#include "RouteHandler.h"
#include "Data/SampleNavdata.cpp"
#include "Helpers/RouteHandlerTestHelpers.cpp"
#include "types/ParsedRoute.h"
#include "types/ParsingError.h"
#include "types/Waypoint.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <optional>
#include <iostream>
using namespace RouteParser;

namespace RouteHandlerTests
{
  class RouteHandlerTest : public ::testing::Test
  {
  protected:
    RouteHandler handler;

    void SetUp() override
    {
      handler = RouteHandler();
      // Any setup needed before each test
      const std::function<void(const char *, const char *)> logFunc =
          [](const char *msg, const char *level)
      {
        // fmt::print(fg(fmt::color::yellow), "[{}] {}\n", level, msg);
        // Ignore log messages in tests
      };
      handler.Bootstrap(logFunc, "", Data::SmallProceduresList, "testdata/airways.db");
    }
  };

  TEST_F(RouteHandlerTest, EmptyRoute)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute("", "KSFO", "KLAX");

    EXPECT_TRUE(parsedRoute.waypoints.empty());
    EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::ROUTE_EMPTY, 1);

    EXPECT_EQ(parsedRoute.errors[0].type, ParsingErrorType::ROUTE_EMPTY);
    EXPECT_EQ(parsedRoute.totalTokens, 0);
  }

  TEST_F(RouteHandlerTest, BasicRouteWithSIDAndSTAR)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R", "ZSNJ", "VHHH");

    EXPECT_BASIC_ROUTE(parsedRoute);
    EXPECT_EQ(parsedRoute.rawRoute,
              "TES61X/06 TESIG A470 DOTMI V512 ABBEY ABBEY3A/07R");
    EXPECT_EQ(parsedRoute.totalTokens, 7);
  }

  TEST_F(RouteHandlerTest, BasicRouteWithSIDAndSTARNotTraversable)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "ABBEY V512 DOTMI A470 TESIG", "ZSNJ", "VHHH");

    EXPECT_EQ(parsedRoute.rawRoute,
              "ABBEY V512 DOTMI A470 TESIG");
    EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::INVALID_AIRWAY_DIRECTION, 2);
    EXPECT_EQ(parsedRoute.totalTokens, 5);
  }

  TEST_F(RouteHandlerTest, BasicRouteNoSIDAndStarNotTraversable)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "TES61X/06 ABBEY V512 DOTMI A470 TESIG ABBEY3A/07R", "ZSNJ", "VHHH");

    EXPECT_EQ(parsedRoute.rawRoute,
              "TES61X/06 ABBEY V512 DOTMI A470 TESIG ABBEY3A/07R");
    EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::INVALID_AIRWAY_DIRECTION, 2);
    EXPECT_EQ(parsedRoute.totalTokens, 7);
  }

  TEST_F(RouteHandlerTest, RouteWithSpacesAndColons)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        " TES61X/06 TESIG      A470 DOTMI  V512 : :ABBEY ABBEY3A/07R  ", "ZSNJ",
        "VHHH");

    EXPECT_BASIC_ROUTE(parsedRoute);
    EXPECT_EQ(parsedRoute.totalTokens, 7);
  }

  TEST_F(RouteHandlerTest, UnknownWaypoint)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "KSFO BLUE INVALID_WPT TESIG KLAX", "KSFO", "KLAX");

    EXPECT_FALSE(parsedRoute.errors.empty());
    EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                               1);
  }

  TEST_F(RouteHandlerTest, DepartureArrivalRunways)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "KSFO/28L BLUE DCT PAINT KLAX/24R ", "KSFO", "KLAX");

    EXPECT_EQ(parsedRoute.departureRunway, "28L");
    EXPECT_EQ(parsedRoute.arrivalRunway, "24R");
    EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 0);
    EXPECT_EQ(parsedRoute.totalTokens, 5);
  }

  TEST_F(RouteHandlerTest, DepartureArrivalRunwaysWithMismatchingICAO)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "KSFO/28L BLUE DCT PAINT KLAX/24R ", "RJTT", "LFPO");

    EXPECT_EQ(parsedRoute.departureRunway, std::nullopt);
    EXPECT_EQ(parsedRoute.arrivalRunway, std::nullopt);
    EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 2);
    EXPECT_EQ(parsedRoute.totalTokens, 5);
  }

  TEST_F(RouteHandlerTest, ChangeOfFlightRule)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "VFR BLUE DCT IFR PAINT", "KSFO", "KLAX");

    ASSERT_EQ(parsedRoute.waypoints.size(), 2);
    EXPECT_EQ(parsedRoute.waypoints[0].GetFlightRule(), FlightRule::VFR);
    EXPECT_EQ(parsedRoute.waypoints[1].GetFlightRule(), FlightRule::IFR);
  }

  TEST_F(RouteHandlerTest, LatLonInRoute)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "BLUE DCT PAINT DCT 5220N03305E", "KSFO", "KLAX");

    ASSERT_EQ(parsedRoute.waypoints.size(), 3);
    ASSERT_EQ(parsedRoute.errors.size(), 0);
    EXPECT_EQ(parsedRoute.waypoints[2].getType(), WaypointType::LATLON);
    EXPECT_EQ(parsedRoute.waypoints[2].getIdentifier(), "5220N03305E");
    EXPECT_EQ(parsedRoute.waypoints[2].getPosition().latitude().degrees(),
              52.333333333333336);
    EXPECT_EQ(parsedRoute.waypoints[2].getPosition().longitude().degrees(),
              33.083333333333336);
  }

  TEST_F(RouteHandlerTest, WaypointsAndLatLonWithPlannedPosition)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "N0284A045 BLUE PAINT/K0200A165 5220N03305E/M082M0160 TESIG/K0200M0650 "
        "PAINT/N0400S0400 BLUE/N0400F165",
        "KSFO", "KLAX");

    ASSERT_EQ(parsedRoute.waypoints.size(), 6);
    ASSERT_EQ(parsedRoute.errors.size(), 0);
    EXPECT_EQ(parsedRoute.totalTokens, 7);

    EXPECT_EQ(parsedRoute.waypoints[0].GetPlannedPosition(), std::nullopt);
    EXPECT_NE(parsedRoute.waypoints[1].GetPlannedPosition(), std::nullopt);
    EXPECT_NE(parsedRoute.waypoints[2].GetPlannedPosition(), std::nullopt);
    EXPECT_NE(parsedRoute.waypoints[3].GetPlannedPosition(), std::nullopt);
    EXPECT_NE(parsedRoute.waypoints[4].GetPlannedPosition(), std::nullopt);
    EXPECT_NE(parsedRoute.waypoints[5].GetPlannedPosition(), std::nullopt);

    // PAINT/K0200A165
    EXPECT_EQ(parsedRoute.waypoints[1].GetPlannedPosition()->plannedAltitude,
              16500);
    EXPECT_EQ(parsedRoute.waypoints[1].GetPlannedPosition()->plannedSpeed, 200);
    EXPECT_EQ(parsedRoute.waypoints[1].GetPlannedPosition()->altitudeUnit,
              Units::Distance::FEET);
    EXPECT_EQ(parsedRoute.waypoints[1].GetPlannedPosition()->speedUnit,
              Units::Speed::KMH);

    // 5220N03305E/M082M0160
    EXPECT_EQ(parsedRoute.waypoints[2].GetPlannedPosition()->plannedAltitude,
              1600);
    EXPECT_EQ(parsedRoute.waypoints[2].GetPlannedPosition()->plannedSpeed, 82);
    EXPECT_EQ(parsedRoute.waypoints[2].GetPlannedPosition()->altitudeUnit,
              Units::Distance::METERS);
    EXPECT_EQ(parsedRoute.waypoints[2].GetPlannedPosition()->speedUnit,
              Units::Speed::MACH);

    // TESIG/K0200M065
    EXPECT_EQ(parsedRoute.waypoints[3].GetPlannedPosition()->plannedAltitude,
              6500);
    EXPECT_EQ(parsedRoute.waypoints[3].GetPlannedPosition()->plannedSpeed, 200);
    EXPECT_EQ(parsedRoute.waypoints[3].GetPlannedPosition()->altitudeUnit,
              Units::Distance::METERS);
    EXPECT_EQ(parsedRoute.waypoints[3].GetPlannedPosition()->speedUnit,
              Units::Speed::KMH);

    // PAINT/N0400S0400
    EXPECT_EQ(parsedRoute.waypoints[4].GetPlannedPosition()->plannedAltitude,
              4000);
    EXPECT_EQ(parsedRoute.waypoints[4].GetPlannedPosition()->plannedSpeed, 400);
    EXPECT_EQ(parsedRoute.waypoints[4].GetPlannedPosition()->altitudeUnit,
              Units::Distance::METERS);
    EXPECT_EQ(parsedRoute.waypoints[4].GetPlannedPosition()->speedUnit,
              Units::Speed::KNOTS);

    // BLUE/N0400F165
    EXPECT_EQ(parsedRoute.waypoints[5].GetPlannedPosition()->plannedAltitude,
              16500);
    EXPECT_EQ(parsedRoute.waypoints[5].GetPlannedPosition()->plannedSpeed, 400);
    EXPECT_EQ(parsedRoute.waypoints[5].GetPlannedPosition()->altitudeUnit,
              Units::Distance::FEET);
    EXPECT_EQ(parsedRoute.waypoints[5].GetPlannedPosition()->speedUnit,
              Units::Speed::KNOTS);
  }

  TEST_F(RouteHandlerTest, WaypointsAndLatLonWithInvalidPatternPlannedPosition)
  {
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "N0284A045 BLUE PAINT/K200A0165 5220N03305E/M0082M160 TESIG/K020M050 "
        "PAINT/N400S400 BLUE/N400F0165",
        "KSFO", "KLAX");

    ASSERT_EQ(parsedRoute.waypoints.size(), 6);
    ASSERT_EQ(parsedRoute.errors.size(), 5);
    EXPECT_EQ(parsedRoute.totalTokens, 7);

    EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::INVALID_DATA, 5);
    EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 5);

    EXPECT_EQ(parsedRoute.waypoints[0].GetPlannedPosition(), std::nullopt);
    EXPECT_EQ(parsedRoute.waypoints[1].GetPlannedPosition(), std::nullopt);
    EXPECT_EQ(parsedRoute.waypoints[2].GetPlannedPosition(), std::nullopt);
    EXPECT_EQ(parsedRoute.waypoints[3].GetPlannedPosition(), std::nullopt);
    EXPECT_EQ(parsedRoute.waypoints[4].GetPlannedPosition(), std::nullopt);
    EXPECT_EQ(parsedRoute.waypoints[5].GetPlannedPosition(), std::nullopt);
  }

} // namespace RouteHandlerTests