#include "Parser.h"
#include "Log.h"
#include "Navdata.h"
#include "SidStarParser.h"
#include "Utils.h"
#include "absl/strings/str_split.h"
#include "erkir/geo/sphericalpoint.h"
#include "types/ParsedRoute.h" // Ensure this header is included
#include "types/ParsedRoute.h"
#include "types/ParsingError.h"
#include "types/RouteWaypoint.h"
#include "types/Waypoint.h"
#include <iostream>
#include <optional>
#include <vector>

using namespace RouteParser;

bool ParserHandler::ParseFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                                          std::string token, std::string anchorIcao, bool strict, FlightRule currentFlightRule)
{

    auto res = SidStarParser::FindProcedure(
        token, anchorIcao, index == 0 ? PROCEDURE_SID : PROCEDURE_STAR, index);

    if (res.errors.size() > 0)
    {
        for (const auto &error : res.errors)
        {
            // This function is called twice, once in strict mode and once in non
            // strict mode We don't want to add the same error twice
            if (strict && error.type == UNKNOWN_PROCEDURE)
            {
                continue; // In strict mode, since we only accept procedures in dataset,
                          // this error can be ignored
            }
            Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, error);
        }
    }

    if (!res.procedure && !res.runway && !res.extractedProcedure)
    {
        return false; // We found nothing
    }

    if (strict && !res.extractedProcedure && !res.runway)
    {
        return false; // In strict mode, we only accept procedures in dataset
    }

    if (res.procedure == anchorIcao && res.runway)
    {
        if (index == 0)
        {
            parsedRoute.departureRunway = res.runway;
        }
        else
        {
            parsedRoute.arrivalRunway = res.runway;
        }
        return true; // ICAO matching anchor (origin or destination) + runway,
                     // always valid
    }

    if (index == 0)
    {
        parsedRoute.departureRunway = res.runway;
        parsedRoute.SID = strict && res.extractedProcedure ? res.extractedProcedure->name
                                                           : res.procedure;
    }
    else
    {
        parsedRoute.arrivalRunway = res.runway;
        parsedRoute.STAR = strict && res.extractedProcedure ? res.extractedProcedure->name
                                                            : res.procedure;
    }

    if (res.extractedProcedure)
    {
        Utils::InsertWaypointsAsRouteWaypoints(
            parsedRoute.waypoints, res.extractedProcedure->waypoints, currentFlightRule);
        return true; // Parsed a procedure
    }

    return !strict && (res.procedure || res.runway);
}

bool ParserHandler::ParseWaypoints(ParsedRoute &parsedRoute, int index, std::string token,
                                   std::optional<Waypoint> &previousWaypoint, FlightRule currentFlightRule)
{
    const std::vector<std::string> parts = absl::StrSplit(token, '/');
    std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd = std::nullopt;
    token = parts[0];

    auto waypoint = NavdataObject::FindClosestWaypointTo(token, previousWaypoint);
    if (waypoint)
    {
        if (parts.size() > 1)
        {
            plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
            if (!plannedAltAndSpd)
            {
                parsedRoute.errors.push_back(
                    {INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
                     index, token + '/' + parts[1], PARSE_ERROR});
            }
        }

        RouteWaypoint newWaypoint = Utils::WaypointToRouteWaypoint(
            waypoint.value(), currentFlightRule, plannedAltAndSpd);

        if (!parsedRoute.waypoints.empty())
        {
            AddDirectSegment(parsedRoute, parsedRoute.waypoints.back(), newWaypoint);
        }

        parsedRoute.waypoints.push_back(newWaypoint);
        previousWaypoint = waypoint;
        return true;
    }

    return false;
}

void ParserHandler::AddConnectionSegments(ParsedRoute& parsedRoute,
    const std::string& origin,
    const std::string& destination)
{
    bool needsOriginConnection = !parsedRoute.SID.has_value();
    bool needsDestConnection = !parsedRoute.STAR.has_value();

    if (!needsOriginConnection && !needsDestConnection) {
        return;
    }

    if (needsOriginConnection && !parsedRoute.waypoints.empty()) {
        auto originWaypoint = NavdataObject::FindWaypointByType(origin, AIRPORT);

        if (originWaypoint) {
            RouteWaypoint originRouteWaypoint = Utils::WaypointToRouteWaypoint(
                originWaypoint.value(),
                parsedRoute.waypoints.front().GetFlightRule());

            ParsedRouteSegment segment{
                originRouteWaypoint,
                parsedRoute.waypoints.front(),
                "DCT",
                -1
            };

            parsedRoute.segments.insert(parsedRoute.segments.begin(), segment);

            parsedRoute.errors.push_back({
                NO_PROCEDURE_FOUND,
                "Added direct connection from origin to first waypoint as no procedure found",
                0,
                origin,
                INFO
                });
        }
    }

    if (needsDestConnection && !parsedRoute.waypoints.empty()) {
        auto destWaypoint = NavdataObject::FindWaypointByType(destination, AIRPORT);

        if (destWaypoint) {
            RouteWaypoint destRouteWaypoint = Utils::WaypointToRouteWaypoint(
                destWaypoint.value(),
                parsedRoute.waypoints.back().GetFlightRule());

            ParsedRouteSegment segment{
                parsedRoute.waypoints.back(),
                destRouteWaypoint,
                "DCT",
                -1
            };

            parsedRoute.segments.push_back(segment);

            parsedRoute.errors.push_back({
                NO_PROCEDURE_FOUND,
                "Added direct connection from last waypoint to destination as no procedure found",
                parsedRoute.totalTokens - 1,
                destination,
                INFO
                });
        }
    }
}

std::optional<RouteWaypoint::PlannedAltitudeAndSpeed>
ParserHandler::ParsePlannedAltitudeAndSpeed(int index, std::string rightToken)
{
    // Example is WAYPOINT/N0490F370 (knots) or WAYPOINT/M083F360 (mach) or
    // WAYPOINT/K0880F360 (kmh) For alt F370 is FL feet, S0150 is 1500 meters,
    // A055 is alt 5500, M0610 is alt 6100 meters For speed, K0880 is 880 km/h,
    // M083 is mach 0.83, S0150 is 150 knots
    auto match = ctre::match<RouteParser::Regexes::RoutePlannedAltitudeAndSpeed>(rightToken);
    if (!match)
    {
        return std::nullopt;
    }

    try
    {
        std::string extractedSpeedUnit = match.get<2>().to_string();
        int extractedSpeed = match.get<3>().to_number();
        std::string extractedAltitudeUnit = match.get<7>().to_string();
        int extractedAltitude = match.get<8>().to_number();

        if (extractedSpeedUnit.empty())
        {
            extractedSpeedUnit = match.get<4>().to_string();
            extractedSpeed = match.get<5>().to_number();
        }

        if (extractedAltitudeUnit.empty())
        {
            extractedAltitudeUnit = match.get<9>().to_string();
            extractedAltitude = match.get<10>().to_number();
        }

        Units::Distance altitudeUnit = Units::Distance::FEET;
        if (extractedAltitudeUnit == "M" || extractedAltitudeUnit == "S")
        {
            altitudeUnit = Units::Distance::METERS; // Todo: distinguish between FL and Alt
        }

        Units::Speed speedUnit = Units::Speed::KNOTS;
        if (extractedSpeedUnit == "M")
        {
            speedUnit = Units::Speed::MACH;
        }
        else if (extractedSpeedUnit == "K")
        {
            speedUnit = Units::Speed::KMH;
        }

        // We need to convert the altitude to a full int
        if (altitudeUnit == Units::Distance::FEET)
        {
            extractedAltitude *= 100;
        }
        if (altitudeUnit == Units::Distance::METERS)
        {
            extractedAltitude *= 10; // Convert tens of meters to meters
        }

        return RouteWaypoint::PlannedAltitudeAndSpeed{extractedAltitude, extractedSpeed,
                                                      altitudeUnit, speedUnit};
    }
    catch (const std::exception &e)
    {
        Log::error("Error trying to parse planned speed and altitude {}", e.what());
        return std::nullopt;
    }

    return std::nullopt;
}

ParsedRoute ParserHandler::ParseRawRoute(std::string route, std::string origin,
                                         std::string destination, FlightRule filedFlightRule)
{
    auto parsedRoute = ParsedRoute();
    parsedRoute.rawRoute = route;

    route = Utils::CleanupRawRoute(route);

    if (route.empty())
    {
        parsedRoute.errors.push_back(
            {ROUTE_EMPTY, "Route is empty", 0, "", PARSE_ERROR});
        return parsedRoute;
    }

    const std::vector<std::string> routeParts = absl::StrSplit(route, ' ');
    parsedRoute.totalTokens = static_cast<int>(routeParts.size());

    auto previousWaypoint = NavdataObject::FindWaypointByType(origin, AIRPORT);
    FlightRule currentFlightRule = filedFlightRule;

    // Get list of valid airways once at the start

    for (auto i = 0; i < routeParts.size(); i++)
    {
        const auto token = routeParts[i];

        // Skip empty/special tokens
        if (token.empty() || token == origin || token == destination || token == " " || token == "." || token == "..")
        {
            parsedRoute.totalTokens--;
            continue;
        }

        if (token == "DCT")
        {
            continue;
        }

        if (this->ParseFlightRule(currentFlightRule, i, token))
        {
            continue;
        }

        if (i == 0 && this->ParsePlannedAltitudeAndSpeed(i, token))
        {
            continue;
        }

        // Handle SID/STAR patterns first
        if ((i == 0 || i == routeParts.size() - 1) && token.find("/") != std::string::npos)
        {
            if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                            i == 0 ? origin : destination, true, currentFlightRule))
            {
                continue;
            }

        }

        // Check if token is a known airway
        bool isAirway = NavdataObject::GetAirwayNetwork()->airwayExists(token);

        if (isAirway && i > 0 && i < routeParts.size() - 1 && previousWaypoint.has_value())
        {
            const auto &nextToken = routeParts[i + 1];
            // Verify next token isn't a SID/STAR (no '/')
            if (token.find('/') == std::string::npos && nextToken.find('/') == std::string::npos)
            {
                if (this->ParseAirway(parsedRoute, i, token, previousWaypoint, nextToken,
                                      currentFlightRule))
                {

                    previousWaypoint = NavdataObject::FindClosestWaypointTo(
                        nextToken, previousWaypoint);
                    i++; // Skip the next token since it was the airway endpoint
                    continue;
                }
            }
        }

        // Only try parsing as waypoint if it's not an airway
        if (!isAirway && this->ParseWaypoints(
                             parsedRoute, i, token, previousWaypoint, currentFlightRule))
        {
            continue;
        }

        if (!isAirway && this->ParseLatLon(
                             parsedRoute, i, token, previousWaypoint, currentFlightRule))
        {
            continue;
        }

        // Try SID/STAR again without strict mode if at start/end
        if (i == 0 || i == routeParts.size() - 1)
        {
            if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                            i == 0 ? origin : destination, false, currentFlightRule))
            {
                continue;
            }
        }

        // If we reach here and the token wasn't identified as an airway,
        // it's truly an unknown waypoint
        if (!isAirway)
        {
            parsedRoute.errors.push_back(
                ParsingError{UNKNOWN_WAYPOINT, "Unknown waypoint", i, token});
        }

    }
    this->AddConnectionSegments(parsedRoute, origin, destination);

    return parsedRoute;
}

bool RouteParser::ParserHandler::ParseFlightRule(
    FlightRule &currentFlightRule, int index, std::string token)
{
    if (token == "IFR")
    {
        currentFlightRule = IFR;
        return true;
    }
    else if (token == "VFR")
    {
        currentFlightRule = VFR;
        return true;
    }
    return false;
}

bool RouteParser::ParserHandler::ParseLatLon(ParsedRoute &parsedRoute, int index,
                                             std::string token, std::optional<Waypoint> &previousWaypoint,
                                             FlightRule currentFlightRule)
{
    const std::vector<std::string> parts = absl::StrSplit(token, '/');
    token = parts[0];
    auto match = ctre::match<RouteParser::Regexes::RouteLatLon>(token);
    if (!match)
    {
        return false;
    }

    try
    {
        const auto latDegrees = match.get<1>().to_number();
        const auto latMinutes = match.get<2>().to_optional_number();
        const auto latCardinal = match.get<3>().to_string();

        const auto lonDegrees = match.get<4>().to_number();
        const auto lonMinutes = match.get<5>().to_optional_number();
        const auto lonCardinal = match.get<6>().to_string();

        if (latDegrees > 90 || lonDegrees > 180)
        {
            parsedRoute.errors.push_back({INVALID_DATA, "Invalid lat/lon coordinate",
                                          index, token, PARSE_ERROR});
            return false;
        }

        double decimalDegreesLat = latDegrees + (latMinutes.value_or(0) / 60.0);
        double decimalDegreesLon = lonDegrees + (lonMinutes.value_or(0) / 60.0);
        if (latCardinal == "S")
        {
            decimalDegreesLat *= -1;
        }
        if (lonCardinal == "W")
        {
            decimalDegreesLon *= -1;
        }
        erkir::spherical::Point point(decimalDegreesLat, decimalDegreesLon);
        const Waypoint waypoint = Waypoint{LATLON, token, point, LATLON};

        std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd = std::nullopt;
        if (parts.size() > 1)
        {
            plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
            if (!plannedAltAndSpd)
            {
                // Misformed second part of waypoint data
                parsedRoute.errors.push_back(
                    {INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
                     index, token + '/' + parts[1], PARSE_ERROR});
            }
        }
        parsedRoute.waypoints.push_back(Utils::WaypointToRouteWaypoint(
            waypoint, currentFlightRule, plannedAltAndSpd));
        previousWaypoint = waypoint; // Update the previous waypoint
        return true;
    }
    catch (const std::exception &e)
    {
        parsedRoute.errors.push_back(
            {INVALID_DATA, "Invalid lat/lon coordinate", index, token, PARSE_ERROR});
        Log::error("Error trying to parse lat/lon ({}): {}", token, e.what());
        return false;
    }

    return false;
};

bool ParserHandler::ParseAirway(ParsedRoute &parsedRoute, int index,
                                std::string token, std::optional<Waypoint> &previousWaypoint,
                                std::optional<std::string> nextToken, FlightRule currentFlightRule)
{
    if (!nextToken || !previousWaypoint)
    {
        return false;
    }

    if (token.find('/') != std::string::npos)
    {
        return false;
    }

    if (token.empty() || !std::isalpha(token[0]))
    {
        return false;
    }

    auto nextWaypoint = NavdataObject::FindClosestWaypointTo(nextToken.value(), previousWaypoint);
    if (!nextWaypoint)
    {
        return false;
    }

    auto airwaySegments = NavdataObject::GetAirwayNetwork()->validateAirwayTraversal(
        previousWaypoint.value(), token, nextToken.value(), 99999, navdata);

    for (const auto &error : airwaySegments.errors)
    {
        ParsingError modifiedError = error;
        modifiedError.token = token;
        modifiedError.level = PARSE_ERROR;
        modifiedError.type = error.type;
        Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, modifiedError);
    }

    if (!airwaySegments.segments.empty() && !parsedRoute.waypoints.empty())
    {
        // Convert the last waypoint in our route to RouteWaypoint
        RouteWaypoint fromWaypoint = parsedRoute.waypoints.back();

        // Add segments for each part of the airway
        for (size_t i = 0; i < airwaySegments.segments.size(); ++i)
        {
            const auto &segment = airwaySegments.segments[i];
            RouteWaypoint toWaypoint = Utils::WaypointToRouteWaypoint(
                segment.to, currentFlightRule);

            // Add the waypoint to the waypoints list
            parsedRoute.waypoints.push_back(toWaypoint);

            // Create and add the segment
            ParsedRouteSegment routeSegment{
                fromWaypoint,
                toWaypoint,
                token,
                static_cast<int>(segment.minimum_level)};
            parsedRoute.segments.push_back(routeSegment);

            // Update fromWaypoint for next iteration
            fromWaypoint = toWaypoint;
        }
        return true;
    }

    if (nextWaypoint && !parsedRoute.waypoints.empty())
    {
        // Handle direct routing case
        RouteWaypoint fromWaypoint = parsedRoute.waypoints.back();
        RouteWaypoint toWaypoint = Utils::WaypointToRouteWaypoint(
            *nextWaypoint, currentFlightRule);

        parsedRoute.waypoints.push_back(toWaypoint);

        ParsedRouteSegment routeSegment{
            fromWaypoint,
            toWaypoint,
            "DCT",
            -1};
        parsedRoute.segments.push_back(routeSegment);
        return true;
    }

    return false;
}

void ParserHandler::AddDirectSegment(ParsedRoute &parsedRoute,
                                     const RouteWaypoint &fromWaypoint, const RouteWaypoint &toWaypoint)
{
    ParsedRouteSegment segment{
        fromWaypoint,
        toWaypoint,
        "DCT",
        -1};
    parsedRoute.segments.push_back(segment);
}