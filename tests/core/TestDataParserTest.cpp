#include "AirwayNetwork.h"
#include "Data/SampleNavdata.cpp"
#include "Helpers/RouteHandlerTestHelpers.cpp"
#include "RouteHandler.h"
#include "types/ParsedRoute.h"
#include "types/ParsingError.h"
#include "types/Waypoint.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <iostream>
#include <optional>

#include "RouteHandler.h"
#include "types/Procedure.h"
#include "types/Waypoint.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Helpers/NseParser.hpp"

using namespace RouteParser;

// Function to convert a JSON-like procedure data to RouteParser::Procedure
std::multimap<std::string, RouteParser::Procedure> ConvertToRouteHandlerProcedures(
    const std::vector<std::map<std::string,
        std::variant<std::string, std::vector<std::string>>>>& proceduresData)
{
    std::multimap<std::string, RouteParser::Procedure> procedures;

    for (const auto& obj : proceduresData) {
        RouteParser::Procedure procedure;

        // Extract basic procedure data
        procedure.name = std::get<std::string>(obj.at("name"));
        procedure.icao = std::get<std::string>(obj.at("icao"));
        procedure.runway = std::get<std::string>(obj.at("runway"));

        // Set procedure type
        std::string type = std::get<std::string>(obj.at("type"));
        procedure.type = (type == "SID") ? RouteParser::ProcedureType::PROCEDURE_SID
                                         : RouteParser::ProcedureType::PROCEDURE_STAR;

        // Get airport reference for finding closest waypoints
        auto airportReference = RouteParser::NavdataObject::FindWaypointByType(
            procedure.icao, WaypointType::AIRPORT);

        // Process waypoints
        const auto& points = std::get<std::vector<std::string>>(obj.at("points"));
        for (const auto& pointName : points) {
            auto closestWaypoint = RouteParser::NavdataObject::FindClosestWaypointTo(
                pointName, airportReference);

            if (closestWaypoint.has_value()) {
                procedure.waypoints.push_back(closestWaypoint.value());
            } else {
                std::cout << "Warning: Could not find waypoint '" << pointName
                          << "' for procedure '" << procedure.name << "'" << std::endl;
            }
        }

        // Add the procedure to the map
        procedures.emplace(procedure.name, procedure);
    }

    return procedures;
}

// Test function for the procedure
void TestProcedureConversion()
{
    // Create test procedure data similar to your JSON example
    std::vector<
        std::map<std::string, std::variant<std::string, std::vector<std::string>>>>
        testData = { { { "type", std::string("SID") }, { "icao", std::string("EGLL") },
            { "name", std::string("DET1J") }, { "runway", std::string("09R") },
            { "points",
                std::vector<std::string> { "RW27L", "D130B", "DE34A", "DE29C", "D283T",
                    "D283P", "D283E", "DET" } } } };

    // Convert the test data to procedures
    auto procedures = ConvertToRouteHandlerProcedures(testData);

    // Print the number of converted procedures
    std::cout << "Converted " << procedures.size() << " procedures" << std::endl;

    // Print details of each procedure
    for (const auto& [name, proc] : procedures) {
        std::cout << "Procedure: " << name << std::endl;
        std::cout << "  ICAO: " << proc.icao << std::endl;
        std::cout << "  Runway: " << proc.runway << std::endl;
        std::cout << "  Type: "
                  << (proc.type == ProcedureType::PROCEDURE_SID ? "SID" : "STAR")
                  << std::endl;
        std::cout << "  Waypoints (" << proc.waypoints.size() << "):" << std::endl;

        for (const auto& waypoint : proc.waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                      << " (Lat: " << waypoint.getPosition().latitude().degrees()
                      << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                      << std::endl;
        }
    }

    // Set the procedures in NavdataObject
    NavdataObject::SetProcedures(procedures);
}

namespace RouteHandlerTests {
class TestDataParserTests : public ::testing::Test {
protected:
    static RouteHandler handler;
    static bool initialized;

    // Static SetUp that runs once for the entire test suite
    static void SetUpTestSuite()
    {
        if (!initialized) {
            handler = RouteHandler();

            // Any setup needed once for all tests
            const std::function<void(const char*, const char*)> logFunc
                = [](const char* file, const char* msg) {};

            std::unordered_map<std::string, RouteParser::AirportRunways> airportRunways;
            airportRunways["LFPG"] = {
                { "08R" }, // Departure runways
                { "08R" } // Arrival runways
            };
            handler.GetAirportConfigurator()->UpdateAirportRunways(airportRunways);

            // Initialize the NavdataObject and load the NSE waypoints before
            // bootstrapping
            auto navdata = std::make_shared<RouteParser::NavdataObject>();

            // Load NSE waypoints
            NavdataObject::LoadNseWaypoints(Data::NseWaypointsList, "Test NSE Provider");

            // Now bootstrap the handler with our extended procedures list
            handler.Bootstrap(logFunc, "testdata/navdata.db", {}, "testdata/gng.db");

            // ExtractNseData("testdata/nse-lfxx.json");

            initialized = true;
        }
    }

    void SetUp() override {}

    static void TearDownTestSuite() {}

    void TearDown() override {}
};

// Initialize static members
RouteHandler TestDataParserTests::handler;
bool TestDataParserTests::initialized = false;

// Test that NSE waypoints are correctly loaded and can be found
// TEST_F(TestDataParserTests, TestNseWaypointsLoaded) {
//    // Check that we can find the NSE waypoints
//    auto det = NavdataObject::FindWaypoint("DET");
//    ASSERT_TRUE(det.has_value());
//    EXPECT_EQ(det->getType(), WaypointType::VOR);
//    EXPECT_NEAR(det->getPosition().latitude().degrees(), 51.30400277777778, 0.0001);

//    auto rw27l = NavdataObject::FindWaypoint("RW27L");
//    ASSERT_TRUE(rw27l.has_value());
//    EXPECT_EQ(rw27l->getType(), WaypointType::FIX);
//    EXPECT_NEAR(rw27l->getPosition().latitude().degrees(), 51.46495277777779, 0.0001);

//    // Test that we can find points from the ALESO1H STAR
//    auto cf09l = NavdataObject::FindWaypoint("CF09L");
//    ASSERT_TRUE(cf09l.has_value());
//    EXPECT_NEAR(cf09l->getPosition().longitude().degrees(), -0.7514805555555557,
//    0.0001);
//}

// Test that NSE procedures are correctly loaded
// TEST_F(TestDataParserTests, TestNseProceduresLoaded) {
//    auto procedures = NavdataObject::GetProcedures();

//    // Check DET1J SID
//    auto det1j_range = procedures.equal_range("DET1J");
//    bool found_det1j = false;
//    for (auto it = det1j_range.first; it != det1j_range.second; ++it) {
//        if (it->second.icao == "EGLL" && it->second.runway == "09R") {
//            found_det1j = true;
//            EXPECT_EQ(it->second.waypoints.size(), 8);
//            break;
//        }
//    }
//    EXPECT_TRUE(found_det1j);

//    // Check ALESO1H STAR
//    auto aleso1h_range = procedures.equal_range("ALESO1H");
//    bool found_aleso1h = false;
//    for (auto it = aleso1h_range.first; it != aleso1h_range.second; ++it) {
//        if (it->second.icao == "EGLL" && it->second.runway == "09L") {
//            found_aleso1h = true;
//            EXPECT_EQ(it->second.waypoints.size(), 11);
//            break;
//        }
//    }
//    EXPECT_TRUE(found_aleso1h);
//}

// Test parsing a route with NSE waypoints
//  TEST_F(TestDataParserTests, TestRouteParsingWithNseWaypoints) {
//      // Parse a route that includes NSE waypoints
//      auto parsedRoute = handler.GetParser()->ParseRawRoute("EGLL/09R DET2J/09L DET L6
//      DVR", "EGLL", "EDDM");

//      if (parsedRoute.suggestedSID.has_value()) {
//          std::cout << "Suggested SID: " << parsedRoute.suggestedSID->name
//              << " for runway " << parsedRoute.suggestedDepartureRunway.value_or("NONE")
//              << std::endl;

//          for (auto& waypoint : parsedRoute.suggestedSID.value().waypoints) {
//              std::cout << "    - " << waypoint.getIdentifier()
//                  << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                  << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                  << std::endl;
//          }
//      }

//      // Check for parsing errors
//      PRINT_ALL_PARSING_ERRORS(parsedRoute);

// std::cout << "Parsed Route Segments: " << parsedRoute.rawRoute << std::endl;

//      if (parsedRoute.SID.has_value()) {
//          std::cout << "Actual SID: " << parsedRoute.SID->name
//              << " for runway " << parsedRoute.departureRunway.value_or("NONE") <<
//              std::endl;

//          for (auto& waypoint : parsedRoute.SID.value().waypoints) {
//              std::cout << "    - " << waypoint.getIdentifier()
//                  << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                  << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                  << std::endl;
//          }
//      }

//      // Print the parsed route details for visualization
//      std::cout << "Parsed Route: " << parsedRoute.rawRoute << std::endl;
//      for (const auto& segment : parsedRoute.explicitSegments) {
//          std::cout << "Segment: " << segment.from.getIdentifier() << " to "
//              << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//      }
//  }

// TEST_F(TestDataParserTests, TestArrivalRouteParsing) {
//     auto parsedRoute = handler.GetParser()->ParseRawRoute("PG270 PG276 OPALE KESAX
//     DIMAL ALESO EGLL/09R ALESO1H/09R", "LFPG", "EGLL");

//    if (parsedRoute.suggestedSTAR.has_value()) {
//        std::cout << "Suggested STAR: " << parsedRoute.suggestedSTAR->name
//            << " for runway " << parsedRoute.suggestedArrivalRunway.value_or("NONE") <<
//            std::endl;

//        for (auto& waypoint : parsedRoute.suggestedSTAR.value().waypoints) {
//            std::cout << "    - " << waypoint.getIdentifier()
//                << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                << std::endl;
//        }
//    }

//    if (parsedRoute.STAR.has_value()) {
//        std::cout << "Actual STAR: " << parsedRoute.STAR->name
//            << " for runway " << parsedRoute.arrivalRunway.value_or("NONE") <<
//            std::endl;

//        for (auto& waypoint : parsedRoute.STAR.value().waypoints) {
//            std::cout << "    - " << waypoint.getIdentifier()
//                << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                << std::endl;
//        }
//    }

//    // Check for parsing errors
//    PRINT_ALL_PARSING_ERRORS(parsedRoute);

//    std::cout << "Parsed Route Segments: " << parsedRoute.rawRoute << std::endl;
//    for (const auto& segment : parsedRoute.segments) {
//        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
//            << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//    }
//    // Print the parsed route details for visualization
//    std::cout << "Parsed Explicit Route Segments: " << parsedRoute.rawRoute <<
//    std::endl; for (const auto& segment : parsedRoute.explicitSegments) {
//        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
//            << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//    }
//}

// TEST_F(TestDataParserTests, TestArrivalRouteParsing) {
//     auto parsedRoute = handler.GetParser()->ParseRawRoute("CELTK7 CELTK DCT FRILL DCT
//     TUSKY N321A ELSIR/M083F390 DCT 50N050W 51N040W 51N030W 52N020W DCT LIMRI DCT XETBO
//     DCT ARKIL/N0476F390 DCT ANNET UM25 INGOR/N0433F280 UM25 LUKIP LUKIP9EXMOPAR7E/08R",
//     "KBOS", "LFPG");

//    if (parsedRoute.suggestedSTAR.has_value()) {
//        std::cout << "Suggested STAR: " << parsedRoute.suggestedSTAR->name
//            << " for runway " << parsedRoute.suggestedArrivalRunway.value_or("NONE") <<
//            std::endl;

//        for (auto& waypoint : parsedRoute.suggestedSTAR.value().waypoints) {
//            std::cout << "    - " << waypoint.getIdentifier()
//                << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                << std::endl;
//        }
//    }

//    if (parsedRoute.STAR.has_value()) {
//        std::cout << "Actual STAR: " << parsedRoute.STAR->name
//            << " for runway " << parsedRoute.arrivalRunway.value_or("NONE") <<
//            std::endl;

//        for (auto& waypoint : parsedRoute.STAR.value().waypoints) {
//            std::cout << "    - " << waypoint.getIdentifier()
//                << " (Lat: " << waypoint.getPosition().latitude().degrees()
//                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
//                << std::endl;
//        }
//    }

//    // Check for parsing errors
//    PRINT_ALL_PARSING_ERRORS(parsedRoute);

//    std::cout << "Parsed Route Segments: " << parsedRoute.rawRoute << std::endl;
//    for (const auto& segment : parsedRoute.segments) {
//        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
//            << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//    }
//    // Print the parsed route details for visualization
//    std::cout << "Parsed Explicit Route Segments: " << parsedRoute.rawRoute <<
//    std::endl; for (const auto& segment : parsedRoute.explicitSegments) {
//        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
//            << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//    }
//}

TEST_F(TestDataParserTests, TestArrivalRouteParsing)
{
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "SOBRA2L/18 SOBRA Y180 DIK UN857 TOLVU/N0388F230 UN857 RAPOR/N0394F240 UZ157 "
        "VEDUS",
        "KBOS", "LFPG");

    if (parsedRoute.suggestedSTAR.has_value()) {
        std::cout << "Suggested STAR: " << parsedRoute.suggestedSTAR->name
                  << " for runway " << parsedRoute.suggestedArrivalRunway.value_or("NONE")
                  << std::endl;

        for (auto& waypoint : parsedRoute.suggestedSTAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                      << " (Lat: " << waypoint.getPosition().latitude().degrees()
                      << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                      << std::endl;
        }
    }

    if (parsedRoute.STAR.has_value()) {
        std::cout << "Actual STAR: " << parsedRoute.STAR->name << " for runway "
                  << parsedRoute.arrivalRunway.value_or("NONE") << std::endl;

        for (auto& waypoint : parsedRoute.STAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                      << " (Lat: " << waypoint.getPosition().latitude().degrees()
                      << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                      << std::endl;
        }
    }

    // Check for parsing errors
    PRINT_ALL_PARSING_ERRORS(parsedRoute);

    std::cout << "Parsed Route Segments: " << parsedRoute.rawRoute << std::endl;
    for (const auto& segment : parsedRoute.segments) {
        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
                  << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
    }
    // Print the parsed route details for visualization
    std::cout << "Parsed Explicit Route Segments: " << parsedRoute.rawRoute << std::endl;
    for (const auto& segment : parsedRoute.explicitSegments) {
        std::cout << "Segment: " << segment.from.getIdentifier() << " to "
                  << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
    }
}

// TEST_F(TestDataParserTests, TestExplicitSegmentsGenerated) {
//     // Parse a route that ends with waypoints on a potential STAR
//     auto parsedRoute = handler.GetParser()->ParseRawRoute(
//         "DET Q70 KOK", "EGLL", "LFPG");

//     // Check for parsing errors and print them
//     PRINT_ALL_PARSING_ERRORS(parsedRoute);

//     // The explicit segments should contain a complete route from LFPG to EGLL
//     EXPECT_FALSE(parsedRoute.explicitSegments.empty()) << "Explicit segments should be
//     generated"; EXPECT_FALSE(parsedRoute.explicitWaypoints.empty()) << "Explicit
//     waypoints should be generated";

//     std::cout << "\nExplicit Route Waypoints (" << parsedRoute.explicitWaypoints.size()
//     << "):" << std::endl; for (const auto& waypoint : parsedRoute.explicitWaypoints) {
//         std::cout << "  - " << waypoint.getIdentifier() << std::endl;
//     }

//     std::cout << "\nExplicit Route Segments (" << parsedRoute.explicitSegments.size()
//     << "):" << std::endl; for (const auto& segment : parsedRoute.explicitSegments) {
//         std::cout << "  " << segment.from.getIdentifier() << " -> "
//             << segment.to.getIdentifier() << " via " << segment.airway << std::endl;
//     }

//     //// Check that we have a connection to the destination airport
//     //const auto& lastSegment = parsedRoute.explicitSegments.back();
//     //EXPECT_EQ(lastSegment.to.getIdentifier(), "EGLL")
//     //    << "Last segment should connect to destination airport";

//     //// Check if STAR waypoints were included
//     //bool foundStarWaypoints = false;
//     //for (const auto& waypoint : parsedRoute.explicitWaypoints) {
//     //    if (waypoint.getIdentifier() == "BIG") {
//     //        foundStarWaypoints = true;
//     //        break;
//     //    }
//     //}

//     //EXPECT_TRUE(foundStarWaypoints)
//     //    << "STAR waypoints should be included in explicit waypoints";

//     //// Make sure we have origin and destination airports in the explicit waypoints
//     //bool hasOrigin = false;
//     //bool hasDestination = false;

//     //for (const auto& waypoint : parsedRoute.explicitWaypoints) {
//     //    if (waypoint.getIdentifier() == "LFPG") {
//     //        hasOrigin = true;
//     //    }
//     //    if (waypoint.getIdentifier() == "EGLL") {
//     //        hasDestination = true;
//     //    }
//     //}

// /*     EXPECT_TRUE(hasOrigin) << "Origin airport should be in explicit waypoints";
//     EXPECT_TRUE(hasDestination) << "Destination airport should be in explicit
//     waypoints";*/
// }

TEST_F(TestDataParserTests, BasicRoute)
{
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "BPK7F/27R Q295 SOMVA L938 MAVAS DOSUR P729 TUDLO ", "EGLL", "EKCH");

    std::ostringstream oss;
    for (const auto& segments : parsedRoute.segments) {

        oss << "-----------------------------------" << std::endl;
        oss << "From: " << segments.from.getIdentifier() << " Lat "
            << segments.from.getPosition().latitude().degrees() << " Lon "
            << segments.from.getPosition().longitude().degrees() << std::endl;
        oss << "To: " << segments.to.getIdentifier() << " Lat "
            << segments.to.getPosition().latitude().degrees() << " Lon "
            << segments.to.getPosition().longitude().degrees() << std::endl;
        oss << "Airway: " << segments.airway << std::endl;
        oss << "-----------------------------------" << std::endl;
    }
    std::cout << oss.str();

    PRINT_ALL_PARSING_ERRORS(parsedRoute);
}

} // namespace RouteHandlerTests