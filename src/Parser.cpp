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

const std::regex ParserHandler::sidStarPattern("([A-Z]{2,5}\\d{1,2}[A-Z]?)(?:/([0-9]{2}[LRC]?))?");
const std::regex ParserHandler::altitudeSpeedPattern("N\\d{4}F\\d{3}");

void ParserHandler::CleanupUnrecognizedPatterns(ParsedRoute& parsedRoute, const std::string& origin, const std::string& destination) {
    if (parsedRoute.rawRoute.empty() || parsedRoute.waypoints.empty()) {
        return;
    }

    // Create a regex for altitude/speed pattern
    const std::regex altitudeSpeedPattern("\\bN\\d{4}F\\d{3}\\b");

    // Create a regex for SID/STAR pattern
    const std::regex sidStarPattern("\\b[A-Z]{2,5}\\d{1,2}[A-Z]?(?:/(?:[0-9]{2}[LRC]?))?\\b");

    // Tokenize the raw route
    std::vector<std::string> tokens = absl::StrSplit(parsedRoute.rawRoute, ' ');

    // Mark tokens to remove
    std::vector<size_t> indicesToRemove;

    // PART 1: Handle altitude/speed before first waypoint
    std::string firstWaypointId = parsedRoute.waypoints.front().getIdentifier();

    // Find index of first waypoint
    size_t firstWaypointIdx = tokens.size();
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == firstWaypointId) {
            firstWaypointIdx = i;
            break;
        }
    }

    // Check for altitude/speed patterns before first waypoint
    // Skip the first token (which could be a SID)
    for (size_t i = 1; i < firstWaypointIdx; i++) {
        if (std::regex_match(tokens[i], altitudeSpeedPattern)) {
            indicesToRemove.push_back(i);
        }
    }

    // PART 2: Handle unrecognized SID/STAR patterns
    for (size_t i = 0; i < tokens.size(); i++) {
        // Skip if this is already marked for removal
        if (std::find(indicesToRemove.begin(), indicesToRemove.end(), i) != indicesToRemove.end()) {
            continue;
        }

        // Skip first and last token if they are recognized SID/STAR
        if ((i == 0 && parsedRoute.SID) || (i == tokens.size() - 1 && parsedRoute.STAR)) {
            continue;
        }

        // Check if token matches SID/STAR pattern
        if (std::regex_match(tokens[i], sidStarPattern)) {
            std::string procedureName = tokens[i];
            auto slashPos = procedureName.find('/');
            if (slashPos != std::string::npos) {
                procedureName = procedureName.substr(0, slashPos);
            }

            // Check if this is a real procedure
            bool isProcedureInDatabase = false;
            auto procedures = NavdataObject::GetProcedures();
            auto range = procedures.equal_range(procedureName);
            for (auto it = range.first; it != range.second; ++it) {
                if ((it->second.icao == origin && it->second.type == PROCEDURE_SID) ||
                    (it->second.icao == destination && it->second.type == PROCEDURE_STAR)) {
                    isProcedureInDatabase = true;
                    break;
                }
            }

            // If not a recognized procedure, mark for removal
            if (!isProcedureInDatabase) {
                indicesToRemove.push_back(i);
            }
        }
    }

    // Remove marked tokens in reverse order
    std::sort(indicesToRemove.begin(), indicesToRemove.end());
    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it) {
        tokens.erase(tokens.begin() + *it);

        // Also remove any errors associated with this token
        for (size_t errIdx = 0; errIdx < parsedRoute.errors.size(); ) {
            if (parsedRoute.errors[errIdx].tokenIndex == static_cast<int>(*it)) {
                parsedRoute.errors.erase(parsedRoute.errors.begin() + errIdx);
            }
            else {
                errIdx++;
            }
        }
    }

    // Rebuild the raw route
    parsedRoute.rawRoute = absl::StrJoin(tokens, " ");

    // Correctly update totalTokens by subtracting the number of tokens we removed
    parsedRoute.totalTokens -= static_cast<int>(indicesToRemove.size());
}

bool ParserHandler::ParseFirstAndLastPart(ParsedRoute& parsedRoute, int index,
    std::string token, std::string anchorIcao, bool strict, FlightRule currentFlightRule)
{
    auto res = SidStarParser::FindProcedure(
        token, anchorIcao, index == 0 ? PROCEDURE_SID : PROCEDURE_STAR, index);

    if (res.errors.size() > 0) {
        for (const auto& error : res.errors) {
            // This function is called twice, once in strict mode and once in non
            // strict mode We don't want to add the same error twice
            if (strict && error.type == UNKNOWN_PROCEDURE) {
                continue; // In strict mode, since we only accept procedures in dataset,
                // this error can be ignored
            }
            Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, error);
        }
    }

    if (!res.procedure && !res.runway && !res.extractedProcedure) {
        return false; // We found nothing
    }

    if (strict && !res.extractedProcedure && !res.runway) {
        return false; // In strict mode, we only accept procedures in dataset
    }

    if (res.procedure == anchorIcao && res.runway) {
        if (index == 0) {
            parsedRoute.departureRunway = res.runway;
        } else {
            parsedRoute.arrivalRunway = res.runway;
        }
        return true; // ICAO matching anchor (origin or destination) + runway,
        // always valid
    }

    if (index == 0) {
        parsedRoute.departureRunway = res.runway;

        // Store the procedure if available
        if (res.extractedProcedure) {
            parsedRoute.SID = res.extractedProcedure;
        }
    } else {
        parsedRoute.arrivalRunway = res.runway;

        // Store the procedure if available
        if (res.extractedProcedure) {
            parsedRoute.STAR = res.extractedProcedure;
        }
    }

    // if (res.extractedProcedure)
    //{
    //     Utils::InsertWaypointsAsRouteWaypoints(
    //         parsedRoute.waypoints, res.extractedProcedure->waypoints,
    //         currentFlightRule);
    //     return true; // Parsed a procedure
    // }

    return !strict && (res.procedure || res.runway);
}

bool ParserHandler::ParseWaypoints(ParsedRoute& parsedRoute, int index, std::string token,
    std::optional<Waypoint>& previousWaypoint, FlightRule currentFlightRule)
{
    const std::vector<std::string> parts = absl::StrSplit(token, '/');
    std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd = std::nullopt;
    token = parts[0];

    auto waypoint = NavdataObject::FindClosestWaypointTo(token, previousWaypoint);
    if (waypoint) {
        if (parts.size() > 1) {
            plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
            if (!plannedAltAndSpd) {
                parsedRoute.errors.push_back(
                    { INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
                        index, token + '/' + parts[1], PARSE_ERROR });
            }
        }

        RouteWaypoint newWaypoint = Utils::WaypointToRouteWaypoint(
            waypoint.value(), currentFlightRule, plannedAltAndSpd);

        if (!parsedRoute.waypoints.empty()) {
            RouteWaypoint& prevWaypoint = parsedRoute.waypoints.back();

            // Calculate heading from previous to current waypoint
            double bearing
                = prevWaypoint.getPosition().bearingTo(newWaypoint.getPosition());
            int heading = static_cast<int>(std::round(bearing));

            ParsedRouteSegment segment { prevWaypoint, newWaypoint, "DCT",
                heading, // Set the heading
                -1 };
            parsedRoute.segments.push_back(segment);
        }

        parsedRoute.waypoints.push_back(newWaypoint);
        previousWaypoint = waypoint;
        return true;
    }

    return false;
}
std::optional<RouteWaypoint::PlannedAltitudeAndSpeed>
ParserHandler::ParsePlannedAltitudeAndSpeed(int index, std::string rightToken)
{
    // Example is WAYPOINT/N0490F370 (knots) or WAYPOINT/M083F360 (mach) or
    // WAYPOINT/K0880F360 (kmh) For alt F370 is FL feet, S0150 is 1500 meters,
    // A055 is alt 5500, M0610 is alt 6100 meters For speed, K0880 is 880 km/h,
    // M083 is mach 0.83, S0150 is 150 knots
    auto match
        = ctre::match<RouteParser::Regexes::RoutePlannedAltitudeAndSpeed>(rightToken);
    if (!match) {
        return std::nullopt;
    }

    try {
        std::string extractedSpeedUnit = match.get<2>().to_string();
        int extractedSpeed = match.get<3>().to_number();
        std::string extractedAltitudeUnit = match.get<7>().to_string();
        int extractedAltitude = match.get<8>().to_number();

        if (extractedSpeedUnit.empty()) {
            extractedSpeedUnit = match.get<4>().to_string();
            extractedSpeed = match.get<5>().to_number();
        }

        if (extractedAltitudeUnit.empty()) {
            extractedAltitudeUnit = match.get<9>().to_string();
            extractedAltitude = match.get<10>().to_number();
        }

        Units::Distance altitudeUnit = Units::Distance::FEET;
        if (extractedAltitudeUnit == "M" || extractedAltitudeUnit == "S") {
            altitudeUnit
                = Units::Distance::METERS; // Todo: distinguish between FL and Alt
        }

        Units::Speed speedUnit = Units::Speed::KNOTS;
        if (extractedSpeedUnit == "M") {
            speedUnit = Units::Speed::MACH;
        } else if (extractedSpeedUnit == "K") {
            speedUnit = Units::Speed::KMH;
        }

        // We need to convert the altitude to a full int
        if (altitudeUnit == Units::Distance::FEET) {
            extractedAltitude *= 100;
        }
        if (altitudeUnit == Units::Distance::METERS) {
            extractedAltitude *= 10; // Convert tens of meters to meters
        }

        return RouteWaypoint::PlannedAltitudeAndSpeed { extractedAltitude, extractedSpeed,
            altitudeUnit, speedUnit };
    } catch (const std::exception& e) {
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

    if (route.empty()) {
        parsedRoute.errors.push_back(
            { ROUTE_EMPTY, "Route is empty", 0, "", PARSE_ERROR });
        return parsedRoute;
    }

    const std::vector<std::string> routeParts = absl::StrSplit(route, ' ');
    parsedRoute.totalTokens = static_cast<int>(routeParts.size());
    auto previousWaypoint = NavdataObject::FindWaypointByType(origin, AIRPORT);
    FlightRule currentFlightRule = filedFlightRule;

    for (auto i = 0; i < routeParts.size(); i++) {
        const auto token = routeParts[i];

        // Skip empty/special tokens
        if (token.empty() || token == origin || token == destination || token == " "
            || token == "." || token == "..") {
            parsedRoute.totalTokens--;
            continue;
        }

        if (token == "DCT") {
            continue;
        }

        if (this->ParseFlightRule(currentFlightRule, i, token)) {
            continue;
        }

        if (i == 0 && this->ParsePlannedAltitudeAndSpeed(i, token)) {
            continue;
        }

        // Handle SID/STAR patterns first
        if ((i == 0 || i == routeParts.size() - 1)
            && token.find("/") != std::string::npos) {
            if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                    i == 0 ? origin : destination, true, currentFlightRule)) {
                continue;
            }
        }

        // Check if token is a known airway
        bool isAirway = NavdataObject::GetAirwayNetwork()->airwayExists(token);

        if (isAirway && i > 0 && i < routeParts.size() - 1
            && previousWaypoint.has_value()) {
            const auto& nextToken = routeParts[i + 1];
            // Verify next token isn't a SID/STAR (no '/')
            if (token.find('/') == std::string::npos
                && nextToken.find('/') == std::string::npos) {
                if (this->ParseAirway(parsedRoute, i, token, previousWaypoint, nextToken,
                        currentFlightRule)) {

                    previousWaypoint = NavdataObject::FindClosestWaypointTo(
                        nextToken, previousWaypoint);
                    i++; // Skip the next token since it was the airway endpoint
                    continue;
                }
            }
        }

        // Only try parsing as waypoint if it's not an airway
        if (!isAirway
            && this->ParseWaypoints(
                parsedRoute, i, token, previousWaypoint, currentFlightRule)) {
            continue;
        }

        if (!isAirway
            && this->ParseLatLon(
                parsedRoute, i, token, previousWaypoint, currentFlightRule)) {
            continue;
        }

        // Try SID/STAR again without strict mode if at start/end
        if (i == 0 || i == routeParts.size() - 1) {
            if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                    i == 0 ? origin : destination, false, currentFlightRule)) {
                continue;
            }
        }

        // If we reach here and the token wasn't identified as an airway,
        // it's truly an unknown waypoint
        if (!isAirway) {
            parsedRoute.errors.push_back(
                ParsingError { UNKNOWN_WAYPOINT, "Unknown waypoint", i, token });
        }
    }

    if (!parsedRoute.waypoints.empty()) {
        CleanupUnrecognizedPatterns(parsedRoute, origin, destination);
    }

    // Add procedure suggestions
    SidStarParser::AddSugggestedProcedures(
        parsedRoute, origin, destination, airportConfigurator);

    // Generate explicit segments with SID/STAR and airport connections
    GenerateExplicitSegments(parsedRoute, origin, destination);

    return parsedRoute;
}

bool RouteParser::ParserHandler::ParseFlightRule(
    FlightRule& currentFlightRule, int index, std::string token)
{
    if (token == "IFR") {
        currentFlightRule = IFR;
        return true;
    } else if (token == "VFR") {
        currentFlightRule = VFR;
        return true;
    }
    return false;
}

bool RouteParser::ParserHandler::ParseLatLon(ParsedRoute& parsedRoute, int index,
    std::string token, std::optional<Waypoint>& previousWaypoint,
    FlightRule currentFlightRule)
{
    const std::vector<std::string> parts = absl::StrSplit(token, '/');
    token = parts[0];
    auto match = ctre::match<RouteParser::Regexes::RouteLatLon>(token);
    if (!match) {
        return false;
    }

    try {
        const auto latDegrees = match.get<1>().to_number();
        const auto latMinutes = match.get<2>().to_optional_number();
        const auto latCardinal = match.get<3>().to_string();

        const auto lonDegrees = match.get<4>().to_number();
        const auto lonMinutes = match.get<5>().to_optional_number();
        const auto lonCardinal = match.get<6>().to_string();

        if (latDegrees > 90 || lonDegrees > 180) {
            parsedRoute.errors.push_back({ INVALID_DATA, "Invalid lat/lon coordinate",
                index, token, PARSE_ERROR });
            return false;
        }

        double decimalDegreesLat = latDegrees + (latMinutes.value_or(0) / 60.0);
        double decimalDegreesLon = lonDegrees + (lonMinutes.value_or(0) / 60.0);
        if (latCardinal == "S") {
            decimalDegreesLat *= -1;
        }
        if (lonCardinal == "W") {
            decimalDegreesLon *= -1;
        }
        erkir::spherical::Point point(decimalDegreesLat, decimalDegreesLon);
        const Waypoint waypoint = Waypoint { LATLON, token, point, LATLON };

        std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd
            = std::nullopt;
        if (parts.size() > 1) {
            plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
            if (!plannedAltAndSpd) {
                // Misformed second part of waypoint data
                parsedRoute.errors.push_back(
                    { INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
                        index, token + '/' + parts[1], PARSE_ERROR });
            }
        }

        RouteWaypoint routeWaypoint = Utils::WaypointToRouteWaypoint(
            waypoint, currentFlightRule, plannedAltAndSpd);

        // If this isn't the first waypoint, add a segment with heading
        if (!parsedRoute.waypoints.empty()) {
            RouteWaypoint& prevRouteWaypoint = parsedRoute.waypoints.back();

            // Calculate heading from previous to current waypoint
            double bearing
                = prevRouteWaypoint.getPosition().bearingTo(routeWaypoint.getPosition());
            int heading = static_cast<int>(std::round(bearing));

            ParsedRouteSegment segment { prevRouteWaypoint, routeWaypoint, "DCT",
                heading, // Set the heading
                -1 };
            parsedRoute.segments.push_back(segment);
        }

        parsedRoute.waypoints.push_back(routeWaypoint);
        previousWaypoint = waypoint; // Update the previous waypoint
        return true;
    } catch (const std::exception& e) {
        parsedRoute.errors.push_back(
            { INVALID_DATA, "Invalid lat/lon coordinate", index, token, PARSE_ERROR });
        Log::error("Error trying to parse lat/lon ({}): {}", token, e.what());
        return false;
    }

    return false;
};

bool ParserHandler::ParseAirway(ParsedRoute& parsedRoute, int index, std::string token,
    std::optional<Waypoint>& previousWaypoint, std::optional<std::string> nextToken,
    FlightRule currentFlightRule)
{
    if (!nextToken || !previousWaypoint) {
        return false;
    }

    if (token.find('/') != std::string::npos) {
        return false;
    }

    if (token.empty() || !std::isalpha(token[0])) {
        return false;
    }

    auto nextWaypoint
        = NavdataObject::FindClosestWaypointTo(nextToken.value(), previousWaypoint);
    if (!nextWaypoint) {
        return false;
    }

    auto airwaySegments = NavdataObject::GetAirwayNetwork()->validateAirwayTraversal(
        previousWaypoint.value(), token, nextToken.value(), 99999, navdata);

    for (const auto& error : airwaySegments.errors) {
        ParsingError modifiedError = error;
        modifiedError.token = token;
        modifiedError.level = PARSE_ERROR;
        modifiedError.type = error.type;
        Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, modifiedError);
    }

    if (!airwaySegments.segments.empty() && !parsedRoute.waypoints.empty()) {
        // Convert the last waypoint in our route to RouteWaypoint
        RouteWaypoint fromWaypoint = parsedRoute.waypoints.back();

        // Add segments for each part of the airway
        for (size_t i = 0; i < airwaySegments.segments.size(); ++i) {
            const auto& segment = airwaySegments.segments[i];
            RouteWaypoint toWaypoint
                = Utils::WaypointToRouteWaypoint(segment.to, currentFlightRule);

            // Calculate heading
            double bearing
                = fromWaypoint.getPosition().bearingTo(toWaypoint.getPosition());
            int heading = static_cast<int>(std::round(bearing));

            // Add the waypoint to the waypoints list
            parsedRoute.waypoints.push_back(toWaypoint);

            // Create and add the segment with the calculated heading
            ParsedRouteSegment routeSegment { fromWaypoint, toWaypoint, token,
                heading, // Set the heading
                static_cast<int>(segment.minimum_level) };
            parsedRoute.segments.push_back(routeSegment);

            // Update fromWaypoint for next iteration
            fromWaypoint = toWaypoint;
        }
        return true;
    }

    if (nextWaypoint && !parsedRoute.waypoints.empty()) {
        // Handle direct routing case
        RouteWaypoint fromWaypoint = parsedRoute.waypoints.back();
        RouteWaypoint toWaypoint
            = Utils::WaypointToRouteWaypoint(*nextWaypoint, currentFlightRule);

        double bearing = fromWaypoint.getPosition().bearingTo(toWaypoint.getPosition());
        int heading = static_cast<int>(std::round(bearing));

        parsedRoute.waypoints.push_back(toWaypoint);

        ParsedRouteSegment routeSegment { fromWaypoint, toWaypoint, "DCT", heading, -1 };
        parsedRoute.segments.push_back(routeSegment);
        return true;
    }

    return false;
}

void ParserHandler::AddDirectSegment(ParsedRoute& parsedRoute,
    const RouteWaypoint& fromWaypoint, const RouteWaypoint& toWaypoint)
{
    double bearing = fromWaypoint.getPosition().bearingTo(toWaypoint.getPosition());

    int heading = static_cast<int>(std::round(bearing));

    ParsedRouteSegment segment { fromWaypoint, toWaypoint, "DCT", heading, -1 };
    parsedRoute.segments.push_back(segment);
}

void RouteParser::ParserHandler::GenerateExplicitSegments(
    ParsedRoute& parsedRoute, const std::string& origin, const std::string& destination)
{
    // Always clear previous explicit segments and waypoints
    parsedRoute.explicitSegments.clear();
    parsedRoute.explicitWaypoints.clear();

    FlightRule flightRule = parsedRoute.waypoints.empty()
        ? IFR
        : parsedRoute.waypoints.front().GetFlightRule();

    // Get origin and destination airport waypoints
    auto originOpt = NavdataObject::FindWaypointByType(origin, AIRPORT);
    auto destOpt = NavdataObject::FindWaypointByType(destination, AIRPORT);
    if (!originOpt || !destOpt) {
        // If we can't find the airports, just copy the original route
        parsedRoute.explicitWaypoints = parsedRoute.waypoints;
        parsedRoute.explicitSegments = parsedRoute.segments;
        return;
    }

    RouteWaypoint originRtw
        = Utils::WaypointToRouteWaypoint(originOpt.value(), flightRule);
    RouteWaypoint destRtw = Utils::WaypointToRouteWaypoint(destOpt.value(), flightRule);

    // Helper lambda to add a segment between two waypoints
    auto addSegment = [&](const RouteWaypoint& from, const RouteWaypoint& to,
                          const std::string& airway = "DCT", int minLevel = -1) {
        // Calculate heading
        double bearing = from.getPosition().bearingTo(to.getPosition());
        int heading = static_cast<int>(std::round(bearing));

        parsedRoute.explicitSegments.push_back({ from, to, airway, heading, minLevel });
    };

    // Helper lambda to process a procedure (SID or STAR)
    auto applyProcedure
        = [&](const std::optional<Procedure>& procOpt,
              const std::vector<RouteWaypoint>& routeWpts, bool isSID) -> bool {
        if (!procOpt.has_value() || procOpt->waypoints.empty())
            return false;

        std::vector<RouteWaypoint> procWpts;
        for (const auto& wp : procOpt->waypoints)
            procWpts.push_back(Utils::WaypointToRouteWaypoint(wp, flightRule));

        if (isSID) {
            // For departure, connect the origin to the first SID waypoint.
            addSegment(originRtw, procWpts.front());

            // Look for a connection point in the FP (using reverse iteration to pick the
            // last match)
            size_t sidConnIdx = procWpts.size(); // indicates no connection found yet
            std::string connId;
            for (size_t i = procWpts.size(); i-- > 0;) {
                for (const auto& rwp : routeWpts) {
                    if (rwp.getIdentifier() == procWpts[i].getIdentifier()) {
                        sidConnIdx = i;
                        connId = procWpts[i].getIdentifier();
                        parsedRoute.sidConnectionWaypoint = connId;
                        break;
                    }
                }
                if (sidConnIdx != procWpts.size())
                    break;
            }

            // Add SID waypoints (up to the connection point, if found)
            for (size_t i = 0; i < procWpts.size(); i++) {
                if (sidConnIdx != procWpts.size() && i > sidConnIdx)
                    break;
                if (i > 0)
                    addSegment(procWpts[i - 1], procWpts[i]);
                parsedRoute.explicitWaypoints.push_back(procWpts[i]);
            }

            // Now add remaining FP waypoints after the connection point.
            if (!routeWpts.empty()) {
                size_t routeStart = 0;
                for (size_t i = 0; i < routeWpts.size(); i++) {
                    if (routeWpts[i].getIdentifier() == connId) {
                        routeStart = i + 1;
                        break;
                    }
                }
                if (routeStart < routeWpts.size()
                    && procWpts.back().getIdentifier()
                        != routeWpts[routeStart].getIdentifier()) {
                    addSegment(procWpts.back(), routeWpts[routeStart]);
                }
                for (size_t i = routeStart; i < routeWpts.size(); i++) {
                    if (i > routeStart)
                        addSegment(routeWpts[i - 1], routeWpts[i],
                            (i - 1 < parsedRoute.segments.size()
                                    ? parsedRoute.segments[i - 1].airway
                                    : "DCT"),
                            (i - 1 < parsedRoute.segments.size()
                                    ? parsedRoute.segments[i - 1].minimumLevel
                                    : -1));
                    parsedRoute.explicitWaypoints.push_back(routeWpts[i]);
                }
            }
        } else {
            size_t explicitConnIdx = 0;
            size_t procConnIdx = 0;
            bool found = false;
            // Iterate the explicit waypoints in reverse order.
            for (size_t j = parsedRoute.explicitWaypoints.size(); j-- > 0;) {
                for (size_t i = 0; i < procWpts.size(); i++) {
                    if (parsedRoute.explicitWaypoints[j].getIdentifier()
                        == procWpts[i].getIdentifier()) {
                        explicitConnIdx = j;
                        procConnIdx = i;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
            if (found) {
                parsedRoute.explicitWaypoints.resize(explicitConnIdx + 1);
                parsedRoute.explicitSegments.resize(explicitConnIdx);

                // Then add STAR waypoints after the connection point.
                for (size_t i = procConnIdx + 1; i < procWpts.size(); i++) {
                    RouteWaypoint prev
                        = (i == procConnIdx + 1 ? parsedRoute.explicitWaypoints.back()
                                                : procWpts[i - 1]);
                    addSegment(prev, procWpts[i]);
                    parsedRoute.explicitWaypoints.push_back(procWpts[i]);
                }
            } else {
                addSegment(parsedRoute.explicitWaypoints.back(), procWpts.front());
                for (size_t i = 0; i < procWpts.size(); i++) {
                    if (i > 0)
                        addSegment(procWpts[i - 1], procWpts[i]);
                    parsedRoute.explicitWaypoints.push_back(procWpts[i]);
                }
            }
        }
        return true;
    };

    // Always start with the origin airport.
    parsedRoute.explicitWaypoints.push_back(originRtw);

    // 1. DEPARTURE: Handle SID if available; otherwise, add a direct connection to the
    // first FP waypoint.
    if (!parsedRoute.waypoints.empty()) {
        if (!applyProcedure(
                parsedRoute.SID.has_value() ? parsedRoute.SID : parsedRoute.suggestedSID,
                parsedRoute.waypoints, true)) {
            addSegment(originRtw, parsedRoute.waypoints.front());
            for (size_t i = 0; i < parsedRoute.waypoints.size(); i++) {
                if (i > 0)
                    addSegment(parsedRoute.waypoints[i - 1], parsedRoute.waypoints[i],
                        (i - 1 < parsedRoute.segments.size()
                                ? parsedRoute.segments[i - 1].airway
                                : "DCT"),
                        (i - 1 < parsedRoute.segments.size()
                                ? parsedRoute.segments[i - 1].minimumLevel
                                : -1));
                parsedRoute.explicitWaypoints.push_back(parsedRoute.waypoints[i]);
            }
        }
    }

    // 2. ARRIVAL: Process STAR procedure (using the modified logic to pick the last FP
    // waypoint in the STAR).
    applyProcedure(
        parsedRoute.STAR.has_value() ? parsedRoute.STAR : parsedRoute.suggestedSTAR,
        parsedRoute.explicitWaypoints, false);

    // 3. Ensure the destination is the final waypoint.
    if (parsedRoute.explicitWaypoints.empty()
        || parsedRoute.explicitWaypoints.back().getIdentifier() != destination) {
        RouteWaypoint lastWp = parsedRoute.explicitWaypoints.empty()
            ? originRtw
            : parsedRoute.explicitWaypoints.back();
        addSegment(lastWp, destRtw);
        parsedRoute.explicitWaypoints.push_back(destRtw);
    }
}