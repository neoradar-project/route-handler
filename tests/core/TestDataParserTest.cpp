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
            "DET1J DET L6 DVR L9 KONAN UL607 MATUG NIVNU T847 TUTOV AMASI RUDUS L984 SULUS Z650 TIPAM T703 DEXIT PINQI VENEN MAREG VEDER ALAMU BUD LHSN BUDOP IRLOX STJ DINRO N743 UDROS UM859 KARDE UN644 ROLIN N644 LAGAS M747 SUBUT T916 LEYLA N161 SARIN A368 FKG W188 GOVSA W66 DKO W64 ELAPU JR VYK ELKUR W40 YQG W142 DALIM A593 DPX A470 DALNU W166 ZJ W167 SASAN SAS82A", "LFMN", "GMMN");

        PRINT_ALL_PARSING_ERRORS(parsedRoute);

        // for (const auto &waypoint : parsedRoute.waypoints)
        // {
        //     std::cout << waypoint.getIdentifier() << " ";
        // }

        // std::cout << std::endl;
    }

} // namespace RouteHandlerTests