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
            handler.Bootstrap(logFunc, {}, Data::SmallProceduresList, "testdata/airways.txt", "testdata/isec.txt");
        }
    };

    TEST_F(TestDataParserTests, BasicRoute)
    {
        auto parsedRoute = handler.GetParser()->ParseRawRoute(
            "DET L6 DVR L9 KONAN UL607 KOK MATUG AMASI BOMBI TENLO DEXIT MAREG ALAMU BUDOP UDROS BAFRA ROLIN N644 LAGAS DISKA M737 VERCA M747 SUBUT T923 ERLEV L850 SAGIL NAMAS N35 RODAR A909 BABUM A477 POGON L143 TISIB L141 KAMUD W186 SADAN W187 TUSLI W112 BILDA W619 CHW V67 YBL W199 NIRUV B215 GODON B215 EKETA B208 B208 HFE", "ZSNJ", "VHHH");

        PRINT_ALL_PARSING_ERRORS(parsedRoute);

        EXPECT_EQ(parsedRoute.totalTokens, 59);
    }

} // namespace RouteHandlerTests