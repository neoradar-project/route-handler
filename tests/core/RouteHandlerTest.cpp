#include <gtest/gtest.h>
#include "RouteHandler.h"
#include <algorithm>

using namespace RouteParser;

namespace RouteHandlerTests
{
    class RouteHandlerTest : public ::testing::Test
    {
    protected:
        RouteHandler handler;

        void SetUp() override
        {
            // Any setup needed before each test
        }
    };

    TEST_F(RouteHandlerTest, EmptyRoute)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute("", "KSFO", "KLAX");

        EXPECT_TRUE(parsedRoute.waypoints.empty());
        EXPECT_FALSE(parsedRoute.errors.empty());
        EXPECT_EQ(parsedRoute.errors[0].type, ROUTE_EMPTY);
        EXPECT_EQ(parsedRoute.totalTokens, 0);
    }

    TEST_F(RouteHandlerTest, BasicRouteWithSIDAndSTAR)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute("KSFO SNTNA2 PAINT KMAE KAYAK3 KLAX", "KSFO", "KLAX");

        EXPECT_FALSE(parsedRoute.waypoints.empty());
        EXPECT_TRUE(parsedRoute.errors.empty());
        EXPECT_EQ(parsedRoute.SID, "SNTNA2");
        EXPECT_EQ(parsedRoute.STAR, "KAYAK3");
    }

    TEST_F(RouteHandlerTest, RouteWithSpacesAndColons)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute("KSFO:SNTNA2:PAINT  KMAE   KAYAK3:KLAX", "KSFO", "KLAX");

        EXPECT_FALSE(parsedRoute.waypoints.empty());
        EXPECT_TRUE(parsedRoute.errors.empty());
        EXPECT_EQ(parsedRoute.SID, "SNTNA2");
        EXPECT_EQ(parsedRoute.STAR, "KAYAK3");
    }

    TEST_F(RouteHandlerTest, UnknownWaypoint)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute("KSFO SID1 INVALID_WPT STAR1 KLAX", "KSFO", "KLAX");

        EXPECT_FALSE(parsedRoute.errors.empty());
        auto hasUnknownWaypointError = std::any_of(parsedRoute.errors.begin(), parsedRoute.errors.end(),
                                                   [](const auto &error)
                                                   { return error.type == UNKNOWN_WAYPOINT; });
        EXPECT_TRUE(hasUnknownWaypointError);
    }

    TEST_F(RouteHandlerTest, DepartureArrivalRunways)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute("KSFO SNTNA2.RW28L PAINT KMAE KAYAK3.RW24R KLAX", "KSFO", "KLAX");

        EXPECT_EQ(parsedRoute.departureRunway, "RW28L");
        EXPECT_EQ(parsedRoute.arrivalRunway, "RW24R");
        EXPECT_TRUE(parsedRoute.errors.empty());
    }

}