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

           /* static inline const std::unordered_multimap<std::string, RouteParser::Procedure>
                SmallProceduresList {
                    { "ABBEY3A",
                        RouteParser::Procedure { "ABBEY3A", "07R", "VHHH",
                            RouteParser::ProcedureType::PROCEDURE_STAR,
                            { RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ABBEY",
                                { 51.510000, 0.080000 }) } } },
                    { "TES61X",
                        RouteParser::Procedure { "TES61X", "06", "ZSNJ",
                            RouteParser::ProcedureType::PROCEDURE_SID,
                            { RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TESIG",
                                { 51.380000, -0.150000 }) } } },
                };*/

            handler.Bootstrap(logFunc, "testdata/navdata.db", Data::SmallProceduresList, "testdata/gng.db");
        }
    };

    TEST_F(TestDataParserTests, BasicRoute)
    {
        // auto parsedRoute = handler.GetParser()->ParseRawRoute(
        //     "DET1J DET L6 DVR L9 KONAN UL607 KOK REMBA UL607 MATUG GUBAX KOMIB TONSU Z35 ODOMO LOMKI SAS82A", "LFMN", "LKPR");
        
		const std::string route = " KONAN UL607 MATUG";

        auto parsedRoute = handler.GetParser()->ParseRawRoute(route, "EGLL", "EDDM");
        std::ostringstream oss;
        for (const auto& segments : parsedRoute.segments)
        {
            
            oss << "-----------------------------------" << std::endl;
            oss << "From: " << segments.from.getIdentifier() << " Lat " << segments.from.getPosition().latitude().degrees() << " Lon " << segments.from.getPosition().longitude().degrees() << std::endl;
            oss << "To: " << segments.to.getIdentifier() << " Lat " << segments.to.getPosition().latitude().degrees() << " Lon " << segments.to.getPosition().longitude().degrees() << std::endl;
            oss << "Airway: " << segments.airway << std::endl;
            oss << "-----------------------------------" << std::endl;
		}
        std::cout << oss.str();

        PRINT_ALL_PARSING_ERRORS(parsedRoute);
    }

} // namespace RouteHandlerTests