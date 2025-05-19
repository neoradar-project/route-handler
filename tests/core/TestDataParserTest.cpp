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
std::vector<RouteParser::Procedure> ConvertToRouteHandlerProcedures(
    const std::vector<std::map<std::string,
        std::variant<std::string, std::vector<std::string>>>>& proceduresData)
{
    std::vector<RouteParser::Procedure> procedures;
    procedures.reserve(proceduresData.size());

    for (const auto& obj : proceduresData) {
        RouteParser::Procedure procedure;

        procedure.name = std::get<std::string>(obj.at("name"));
        procedure.icao = std::get<std::string>(obj.at("icao"));
        procedure.runway = std::get<std::string>(obj.at("runway"));

        std::string type = std::get<std::string>(obj.at("type"));
        procedure.type = (type == "SID") ? RouteParser::ProcedureType::PROCEDURE_SID
                                         : RouteParser::ProcedureType::PROCEDURE_STAR;

        auto airportReference = RouteParser::NavdataObject::FindWaypointByType(
            procedure.icao, WaypointType::AIRPORT);

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

        procedures.push_back(procedure);
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
    for (const auto& proc : procedures) {
        std::cout << "Procedure: " << proc.name << std::endl;
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
                { "09R", "08L"}, // Departure runways
                { "09L", "08R" } // Arrival runways
            };
            handler.GetAirportConfigurator()->UpdateAirportRunways(airportRunways);

            // Initialize the NavdataObject and load the NSE waypoints before
            // bootstrapping
            auto navdata = std::make_shared<RouteParser::NavdataObject>();

            // Load NSE waypoints
            NavdataObject::LoadNseWaypoints(Data::NseWaypointsList, "Test NSE Provider");

            // Now bootstrap the handler with our extended procedures list
            handler.Bootstrap(logFunc, "testdata/navdata.db", {}, "testdata/gng.db");

            ExtractNseData("testdata/nse-lfxx.json");

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


TEST_F(TestDataParserTests, TestDeparture1RouteParsing) {
    // Parse a route that includes NSE waypoints

    auto parsedRoute = handler.GetParser()->ParseRawRoute(
    "LANVI DCT BEGAR DCT TRA DCT SUXAN DCT SOVOX DCT KOTOR DCT DOBOT DCT VEBAR DCT NISVA DCT DEDIN DCT AYTEK AYTEK1B/17L", "LFPG", "EDDM");

    std::cout << "Route: " << parsedRoute.rawRoute << std::endl;
    if (parsedRoute.suggestedSID.has_value()) {
        std::cout << "Suggested SID: " << parsedRoute.suggestedSID->name
            << " for runway " << parsedRoute.suggestedDepartureRunway.value_or("NONE")
            << std::endl;

    }


    if (parsedRoute.SID.has_value()) {
        std::cout << "Actual SID: " << parsedRoute.SID->name
            << " for runway " << parsedRoute.departureRunway.value_or("NONE") <<
            std::endl;

    }

}

TEST_F(TestDataParserTests, TestDeparture2RouteParsing) {
    // Parse a route that includes NSE waypoints
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
    "LATRA DCT LAMUT DCT UTUVA/N0447F370 DCT TITVA DCT NOQAS UM728 KOLON DCT LOKDU DCT PELOS DCT IPROM DCT MOROB DCT GIGGI DCT CAR DCT NOLSI/N0416F230 A868 MEDAR", "LFPG", "EDDM");
    std::cout << "Route: " << parsedRoute.rawRoute << std::endl;

    if (parsedRoute.suggestedSID.has_value()) {
        std::cout << "Suggested SID: " << parsedRoute.suggestedSID->name
            << " for runway " << parsedRoute.suggestedDepartureRunway.value_or("NONE")
            << std::endl;

    }


    if (parsedRoute.SID.has_value()) {
        std::cout << "Actual SID: " << parsedRoute.SID->name
            << " for runway " << parsedRoute.departureRunway.value_or("NONE") <<
            std::endl;

    }

 }

TEST_F(TestDataParserTests, TestArrivalRouteParsing1) {
    auto parsedRoute = handler.GetParser()->ParseRawRoute("ETREK UN854 LOGNI DCT MOKIP DCT ARFOZ UN854 TINIL ", "KMCO", "LFPG");
     std::cout << "Route: " << parsedRoute.rawRoute << std::endl;

   if (parsedRoute.suggestedSTAR.has_value()) {
       std::cout << "Suggested STAR: " << parsedRoute.suggestedSTAR->name
           << " for runway " << parsedRoute.suggestedArrivalRunway.value_or("NONE") <<
           std::endl;

       /* for (auto& waypoint : parsedRoute.suggestedSTAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                << " (Lat: " << waypoint.getPosition().latitude().degrees()
                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                << std::endl;
        }*/
   }

   if (parsedRoute.STAR.has_value()) {
       std::cout << "Actual STAR: " << parsedRoute.STAR->name
           << " for runway " << parsedRoute.arrivalRunway.value_or("NONE") <<
           std::endl;

      /*  for (auto& waypoint : parsedRoute.STAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                << " (Lat: " << waypoint.getPosition().latitude().degrees()
                << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                << std::endl;
        }*/
   }
}

TEST_F(TestDataParserTests, TestArrivalRouteParsing2)
{
    auto parsedRoute = handler.GetParser()->ParseRawRoute(
        "SOBRA2L/18 SOBRA Y180 DIK UN857 TOLVU/N0356F230 UN857 RAPOR/N0363F240 UZ157 VEDUS", "KLAX", "LFPG");
    std::cout << "Route: " << parsedRoute.rawRoute << std::endl;

    if (parsedRoute.suggestedSTAR.has_value()) {
        std::cout << "Suggested STAR: " << parsedRoute.suggestedSTAR->name
                  << " for runway " << parsedRoute.suggestedArrivalRunway.value_or("NONE")
                  << std::endl;

        /*for (auto& waypoint : parsedRoute.suggestedSTAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                      << " (Lat: " << waypoint.getPosition().latitude().degrees()
                      << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                      << std::endl;
        }*/
    }

    if (parsedRoute.STAR.has_value()) {
        std::cout << "Actual STAR: " << parsedRoute.STAR->name << " for runway "
                  << parsedRoute.arrivalRunway.value_or("NONE") << std::endl;

       /* for (auto& waypoint : parsedRoute.STAR.value().waypoints) {
            std::cout << "    - " << waypoint.getIdentifier()
                      << " (Lat: " << waypoint.getPosition().latitude().degrees()
                      << ", Lon: " << waypoint.getPosition().longitude().degrees() << ")"
                      << std::endl;
        }*/
    }
}

} // namespace RouteHandlerTests