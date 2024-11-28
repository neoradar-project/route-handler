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
            handler.Bootstrap(logFunc, "", Data::SmallProceduresList, "testdata/gng.db");
        }
    };

    TEST_F(TestDataParserTests, BasicRoute)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute(
            "DET L6 DVR L9 KONAN UL607 KOK MATUG GUBAX BOREP ENITA PEPIK BALAP NARKA DENAK DINRO UDROS UN743 GAKSU UN644 ROLIN N644 TETRO BARAD M747 LUSAL N199 RASAM L88 METKA B476 TIMGA A480 OTBOR N147 GENGA N143 RUDIZ N161 SARIN A368 FKG W188 GOVSA W66 DKO W64 NUTLO B208 CGO W129 KAMDA W128 FYG B208 HFE R343 SASAN", "EGLL", "ZSPD");

        PRINT_ALL_PARSING_ERRORS(parsedRoute);

        EXPECT_EQ(parsedRoute.totalTokens, 58);
    }

} // namespace RouteHandlerTests