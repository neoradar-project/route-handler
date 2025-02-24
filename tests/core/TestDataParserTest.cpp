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
#include "AirwayNetwork.h"
using namespace RouteParser;

namespace RouteHandlerTests
{
    class TestDataParserTests : public ::testing::Test
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
            handler.Bootstrap(logFunc, "testdata/navdata.db", Data::SmallProceduresList, "testdata/gng.db");
        }
    };

    TEST_F(TestDataParserTests, BasicRoute)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute(
            "DET1J DET L6 DVR L9 KONAN UL607 KOK REMBA UL607 MATUG GUBAX KOMIB TONSU Z35 ODOMO LOMKI SAS82A", "LFMN", "LKPR");

        PRINT_ALL_PARSING_ERRORS(parsedRoute);
    }

} // namespace RouteHandlerTests