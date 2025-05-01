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

const std::regex ParserHandler::sidStarPattern(
    "([A-Z]{2,5}\\d{1,2}[A-Z]?)(?:/([0-9]{2}[LRC]?))?");
const std::regex ParserHandler::altitudeSpeedPattern("N\\d{4}F\\d{3}");

void ParserHandler::CleanupUnrecognizedPatterns(
    ParsedRoute& parsedRoute, const std::string& origin, const std::string& destination)
{
    if (parsedRoute.rawRoute.empty() || parsedRoute.waypoints.empty()) {
        return;
    }

    int tokensRemoved = 0;
    std::string cleanedRoute = parsedRoute.rawRoute;

    // PART 1: Find and remove altitude/speed patterns before the first waypoint
    if (!parsedRoute.waypoints.empty()) {
        std::string firstWaypointId = parsedRoute.waypoints.front().getIdentifier();

        // Find the position of the first waypoint in the route
        size_t firstWaypointPos = cleanedRoute.find(firstWaypointId);
        if (firstWaypointPos != std::string::npos) {
            // Get the part of the route before the first waypoint
            std::string beforeFirstWaypoint = cleanedRoute.substr(0, firstWaypointPos);

            // Define altitude/speed pattern and find it in the route
            std::regex altitudeSpeedPattern("\\bN\\d{4}F\\d{3}\\b");

            // Use regex_search to find the pattern
            std::smatch matches;
            while (
                std::regex_search(beforeFirstWaypoint, matches, altitudeSpeedPattern)) {
                // Get the matched string
                std::string matchedStr = matches.str();

                // Find and remove the match from the route
                size_t pos = cleanedRoute.find(matchedStr);
                if (pos != std::string::npos) {
                    cleanedRoute.replace(pos, matchedStr.length(), "");
                    tokensRemoved++;

                    // Remove any errors associated with this token
                    for (size_t errIdx = 0; errIdx < parsedRoute.errors.size();) {
                        if (parsedRoute.errors[errIdx].token == matchedStr) {
                            parsedRoute.errors.erase(parsedRoute.errors.begin() + errIdx);
                        } else {
                            errIdx++;
                        }
                    }
                }

                // Update beforeFirstWaypoint for the next search
                size_t matchPos = beforeFirstWaypoint.find(matchedStr);
                beforeFirstWaypoint
                    = beforeFirstWaypoint.substr(matchPos + matchedStr.length());
            }
        }
    }

    // PART 2: Find and remove unrecognized SID/STAR patterns
    std::regex sidStarPattern("\\b[A-Z]{2,5}\\d{1,2}[A-Z]?(?:/(?:[0-9]{2}[LRC]?))?\\b");
    std::string routeCopy = cleanedRoute;
    std::smatch matches;

    // Collect all SID/STAR-like patterns
    std::vector<std::string> sidStarTokens;
    while (std::regex_search(routeCopy, matches, sidStarPattern)) {
        sidStarTokens.push_back(matches.str());
        routeCopy = matches.suffix();
    }

    // Process each SID/STAR-like token
    for (const auto& token : sidStarTokens) {
        // Skip if it's the first or last token and matches a recognized procedure
        if ((cleanedRoute.find(token) == 0 && parsedRoute.SID)
            || (cleanedRoute.rfind(token) == cleanedRoute.length() - token.length()
                && parsedRoute.STAR)) {
            continue;
        }

        // Extract procedure name (without runway)
        std::string procedureName = token;
        auto slashPos = procedureName.find('/');
        if (slashPos != std::string::npos) {
            procedureName = procedureName.substr(0, slashPos);
        }

        // Check if this is a real procedure
        bool isProcedureInDatabase = false;
        auto procedures = NavdataObject::GetProcedures();
        auto range = procedures.equal_range(procedureName);
        for (auto it = range.first; it != range.second; ++it) {
            if ((it->second.icao == origin && it->second.type == PROCEDURE_SID)
                || (it->second.icao == destination
                    && it->second.type == PROCEDURE_STAR)) {
                isProcedureInDatabase = true;
                break;
            }
        }

        // If not a recognized procedure, remove it
        if (!isProcedureInDatabase) {
            size_t pos = 0;
            while (
                (pos = cleanedRoute.find(" " + token + " ", pos)) != std::string::npos) {
                cleanedRoute.replace(pos, token.length() + 2, " ");
                tokensRemoved++;
            }

            // Check beginning of string
            if (cleanedRoute.find(token + " ") == 0) {
                cleanedRoute.replace(0, token.length() + 1, "");
                tokensRemoved++;
            }

            // Check end of string
            if (cleanedRoute.length() >= token.length() + 1
                && cleanedRoute.rfind(" " + token)
                    == cleanedRoute.length() - token.length() - 1) {
                cleanedRoute.replace(
                    cleanedRoute.length() - token.length() - 1, token.length() + 1, "");
                tokensRemoved++;
            }

            // Remove any errors associated with this token
            for (size_t errIdx = 0; errIdx < parsedRoute.errors.size();) {
                if (parsedRoute.errors[errIdx].token == token) {
                    parsedRoute.errors.erase(parsedRoute.errors.begin() + errIdx);
                } else {
                    errIdx++;
                }
            }
        }
    }

    // Clean up multiple spaces and update the route
    while (cleanedRoute.find("  ") != std::string::npos) {
        cleanedRoute.replace(cleanedRoute.find("  "), 2, " ");
    }

    // Trim leading/trailing spaces
    if (!cleanedRoute.empty() && cleanedRoute.front() == ' ') {
        cleanedRoute.erase(0, 1);
    }
    if (!cleanedRoute.empty() && cleanedRoute.back() == ' ') {
        cleanedRoute.pop_back();
    }

    parsedRoute.rawRoute = cleanedRoute;

    // Update the totalTokens
    parsedRoute.totalTokens -= tokensRemoved;
}

bool ParserHandler::ParseFirstAndLastPart(ParsedRoute& parsedRoute, int index,
    std::string token, std::string anchorIcao, bool strict,
    std::string& tokenToRemove, FlightRule currentFlightRule)
{
    // Initialize output parameter
    tokenToRemove = "";

    // Try to find the best match for this token
    auto res = SidStarParser::FindProcedure(
        token, anchorIcao, index == 0 ? PROCEDURE_SID : PROCEDURE_STAR, index);

    // First, handle runway mismatch - this is a critical error that should mark the token for removal immediately
    for (const auto& error : res.errors) {
        if (error.type == PROCEDURE_RUNWAY_MISMATCH) {
            // For runway mismatch, always remove this token
            tokenToRemove = token;

            // Add error to parsed route
            Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, error);

            // Return false to indicate we didn't successfully parse this token
            return false;
        }

        // Process other errors
        if (strict && error.type == UNKNOWN_PROCEDURE) {
            continue;  // Ignore unknown procedure errors in strict mode
        }
        Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, error);
    }

    // In strict mode, require a procedure match
    if (strict && !res.extractedProcedure && !res.runway) {
        return false;
    }

    // Determine the quality of this match
    int matchQuality = 0;
    if (res.extractedProcedure && res.runway) {
        matchQuality = 3;  // Best: procedure + matching runway
    }
    else if (res.extractedProcedure) {
        matchQuality = 2;  // Good: procedure but no runway specified
    }
    else if (res.runway) {
        matchQuality = 1;  // Basic: just airport with runway
    }

    // No valid match found
    if (matchQuality == 0) {
        return false;
    }

    // Check if we're replacing an existing SID/STAR
    if (index == 0) {
        // SID case - determine quality of existing match
        int existingQuality = 0;
        if (parsedRoute.SID.has_value() && parsedRoute.departureRunway.has_value()) {
            existingQuality = 3;  // Has procedure + runway
        }
        else if (parsedRoute.SID.has_value()) {
            existingQuality = 2;  // Has procedure but no runway
        }
        else if (parsedRoute.departureRunway.has_value()) {
            existingQuality = 1;  // Has runway but no procedure
        }

        // If existing match is better or equal quality, keep it and remove this token
        if (existingQuality >= matchQuality) {
            tokenToRemove = token;
            return false;  // Don't update the route
        }

        // New match is better - find and mark the old token for removal
        if (existingQuality > 0) {
            const std::vector<std::string> routeParts = absl::StrSplit(parsedRoute.rawRoute, ' ');
            for (const auto& routePart : routeParts) {
                if (routePart.find('/') != std::string::npos && routePart != token) {
                    // Check if this is the old SID/runway token
                    auto oldMatch = SidStarParser::FindProcedure(
                        routePart, anchorIcao, PROCEDURE_SID, 0);

                    bool isSidToken = false;
                    if (parsedRoute.SID.has_value() && oldMatch.extractedProcedure &&
                        oldMatch.extractedProcedure->name == parsedRoute.SID->name) {
                        isSidToken = true;
                    }

                    bool isRunwayToken = false;
                    if (parsedRoute.departureRunway.has_value() && oldMatch.runway &&
                        oldMatch.runway == parsedRoute.departureRunway) {
                        isRunwayToken = true;
                    }

                    // Mark for removal if this token contains the existing SID or runway
                    if (isSidToken || isRunwayToken) {
                        tokenToRemove = routePart;
                        break;
                    }
                }
            }
        }
    }
    else {
        // STAR case - similar logic to SID case
        int existingQuality = 0;
        if (parsedRoute.STAR.has_value() && parsedRoute.arrivalRunway.has_value()) {
            existingQuality = 3;
        }
        else if (parsedRoute.STAR.has_value()) {
            existingQuality = 2;
        }
        else if (parsedRoute.arrivalRunway.has_value()) {
            existingQuality = 1;
        }

        if (existingQuality >= matchQuality) {
            tokenToRemove = token;
            return false;
        }

        if (existingQuality > 0) {
            const std::vector<std::string> routeParts = absl::StrSplit(parsedRoute.rawRoute, ' ');
            for (const auto& routePart : routeParts) {
                if (routePart.find('/') != std::string::npos && routePart != token) {
                    auto oldMatch = SidStarParser::FindProcedure(
                        routePart, anchorIcao, PROCEDURE_STAR, routeParts.size() - 1);

                    bool isStarToken = false;
                    if (parsedRoute.STAR.has_value() && oldMatch.extractedProcedure &&
                        oldMatch.extractedProcedure->name == parsedRoute.STAR->name) {
                        isStarToken = true;
                    }

                    bool isRunwayToken = false;
                    if (parsedRoute.arrivalRunway.has_value() && oldMatch.runway &&
                        oldMatch.runway == parsedRoute.arrivalRunway) {
                        isRunwayToken = true;
                    }

                    if (isStarToken || isRunwayToken) {
                        tokenToRemove = routePart;
                        break;
                    }
                }
            }
        }
    }

    // Update the parsed route with the new match
    if (index == 0) {
        // SID case
        parsedRoute.departureRunway = res.runway;
        if (res.extractedProcedure) {
            parsedRoute.SID = res.extractedProcedure;
        }
    }
    else {
        // STAR case
        parsedRoute.arrivalRunway = res.runway;
        if (res.extractedProcedure) {
            parsedRoute.STAR = res.extractedProcedure;
        }
    }

    return true;
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

            ParsedRouteSegment segment{ prevWaypoint, newWaypoint, "DCT",
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
    if (std::regex_match(rightToken, std::regex("\\d{2}[LCR]?"))) {
        return std::nullopt;
    }

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
        }
        else if (extractedSpeedUnit == "K") {
            speedUnit = Units::Speed::KMH;
        }

        // We need to convert the altitude to a full int
        if (altitudeUnit == Units::Distance::FEET) {
            extractedAltitude *= 100;
        }
        if (altitudeUnit == Units::Distance::METERS) {
            extractedAltitude *= 10; // Convert tens of meters to meters
        }

        return RouteWaypoint::PlannedAltitudeAndSpeed{ extractedAltitude, extractedSpeed,
            altitudeUnit, speedUnit };
    }
    catch (const std::exception& e) {
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

    // Track tokens to remove (for overridden SID/STAR procedures)
    std::vector<std::string> tokensToRemove;

    // Track if we've found the first waypoint (to stop SID parsing)
    bool foundFirstWaypoint = false;
    // Track the last waypoint index (to start STAR parsing from there)
    int lastWaypointIndex = -1;

    // First pass: Process SID tokens (at beginning), waypoints and airways
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

        // Check if this token looks like a destination airport with runway
        if (token.length() >= 7 && token.substr(0, 4) == destination && token.find('/') != std::string::npos) {
            // This is a destination code with runway, process it as a STAR candidate later
            continue;
        }

        // Handle potential SID procedure (before we find the first waypoint)
        if (!foundFirstWaypoint && token.find("/") != std::string::npos) {
            std::string tokenToRemove;
            if (this->ParseFirstAndLastPart(parsedRoute, 0, token, origin, true, tokenToRemove, currentFlightRule)) {
                if (!tokenToRemove.empty()) {
                    tokensToRemove.push_back(tokenToRemove);
                }
                continue;
            }
            else if (!tokenToRemove.empty()) {
                // Even if ParseFirstAndLastPart returned false, it might have marked a token for removal
                tokensToRemove.push_back(tokenToRemove);
                continue;
            }
        }

        // Check if token is a known airway
        bool isAirway = NavdataObject::GetAirwayNetwork()->airwayExists(token);

        // Check if token looks like a STAR (contains at least one letter, one digit, and possibly a slash)
        bool isPotentialStar = false;
        if (token.find('/') != std::string::npos) {
            // Check if this matches a PROCEDURE/RUNWAY or AIRPORT/RUNWAY pattern
            std::regex procedureOrAirportPattern("(?:\\b[A-Z]{5}\\d{1}[A-Z]?|\\b[A-Z]{4})\\/\\d{2}[LRC]?");
            isPotentialStar = std::regex_match(token, procedureOrAirportPattern);
        }

        // Try parsing as waypoint if it's not an airway or potential STAR
        if (!isAirway && !isPotentialStar &&
            this->ParseWaypoints(parsedRoute, i, token, previousWaypoint, currentFlightRule)) {
            foundFirstWaypoint = true;
            lastWaypointIndex = i;
            continue;
        }

        // Try parsing as lat/lon coordinates if not an airway or potential STAR
        if (!isAirway && !isPotentialStar &&
            this->ParseLatLon(parsedRoute, i, token, previousWaypoint, currentFlightRule)) {
            foundFirstWaypoint = true;
            lastWaypointIndex = i;
            continue;
        }

        // Handle airway (after checking for waypoint)
        if (isAirway && i > 0 && i < routeParts.size() - 1 && previousWaypoint.has_value()) {
            const auto& nextToken = routeParts[i + 1];
            // Verify next token isn't a SID/STAR (no '/')
            if (token.find('/') == std::string::npos && nextToken.find('/') == std::string::npos &&
                !std::regex_match(nextToken, std::regex("[A-Z]{2,5}\\d{1,2}[A-Z]?"))) {
                if (this->ParseAirway(parsedRoute, i, token, previousWaypoint, nextToken, currentFlightRule)) {
                    previousWaypoint = NavdataObject::FindClosestWaypointTo(nextToken, previousWaypoint);
                    lastWaypointIndex = i + 1;
                    i++; // Skip the next token since it was the airway endpoint
                    continue;
                }
            }
        }

        // Try SID again without strict mode if at beginning
        if (i == 0) {
            std::string tokenToRemove;
            if (this->ParseFirstAndLastPart(parsedRoute, i, token, origin, false, tokenToRemove, currentFlightRule)) {
                if (!tokenToRemove.empty()) {
                    tokensToRemove.push_back(tokenToRemove);
                }
                continue;
            }
            else if (!tokenToRemove.empty()) {
                tokensToRemove.push_back(tokenToRemove);
                continue;
            }
        }

        // If we reach here and the token wasn't identified as an airway or waypoint,
        // keep going - we'll process unknown and STAR tokens in the second pass
    }

    // Second pass: Collect all STAR-like tokens and find the best one
    if (lastWaypointIndex >= 0) {
        // First collect all STAR-like tokens that come after the last waypoint
        std::vector<std::pair<int, std::string>> starCandidates;
        for (auto i = lastWaypointIndex + 1; i < routeParts.size(); i++) {
            const auto& token = routeParts[i];

            // Add destination airport codes with runway
            if (token.length() >= 7 && token.substr(0, 4) == destination && token.find('/') != std::string::npos) {
                starCandidates.push_back({ i, token });
                continue;
            }

            // Add tokens that look like procedures
            if (token.find('/') != std::string::npos ||
                std::regex_match(token, std::regex("[A-Z]{2,5}\\d{1,2}[A-Z]?"))) {
                starCandidates.push_back({ i, token });
            }
        }

        // Process each STAR candidate, tracking the best one
        int bestStarQuality = 0;
        std::string bestStarToken;

        for (const auto& [idx, token] : starCandidates) {
            std::string tokenToRemove;

            // Get the procedure match result directly
            auto procedureResult = SidStarParser::FindProcedure(token, destination, PROCEDURE_STAR, idx);

            // Add any errors from the procedure result
            for (const auto& error : procedureResult.errors) {
                Utils::InsertParsingErrorIfNotDuplicate(parsedRoute.errors, error);

                // If we have an airport mismatch, mark the token for removal
                if (error.type == PROCEDURE_AIRPORT_MISMATCH) {
                    tokensToRemove.push_back(token);
                }
            }

            // Continue with existing ParseFirstAndLastPart logic
            bool parsed = this->ParseFirstAndLastPart(parsedRoute, idx, token, destination, true, tokenToRemove, currentFlightRule);

            // Determine the quality of this match
            int matchQuality = 0;
            if (parsed) {
                // This token became our STAR or runway
                if (parsedRoute.STAR.has_value() && parsedRoute.arrivalRunway.has_value()) {
                    matchQuality = 3;  // Both STAR and runway
                }
                else if (parsedRoute.STAR.has_value()) {
                    matchQuality = 2;  // STAR but no runway
                }
                else if (parsedRoute.arrivalRunway.has_value()) {
                    matchQuality = 1;  // Runway but no STAR
                }
            }

            // Keep track of the best STAR token
            if (matchQuality > bestStarQuality) {
                bestStarQuality = matchQuality;
                bestStarToken = token;
            }

            // Add any token marked for removal
            if (!tokenToRemove.empty()) {
                tokensToRemove.push_back(tokenToRemove);
            }
        }

        // Remove all STAR candidates except the best one
        for (const auto& [idx, token] : starCandidates) {
            if (token != bestStarToken && bestStarQuality > 0) {
                tokensToRemove.push_back(token);
            }
        }

        // Process any unknown tokens after the last waypoint
        for (auto i = lastWaypointIndex + 1; i < routeParts.size(); i++) {
            const auto& token = routeParts[i];

            // Skip tokens that will be removed or have already been processed
            if (std::find(tokensToRemove.begin(), tokensToRemove.end(), token) != tokensToRemove.end() ||
                token.empty() || token == "DCT" || (bestStarQuality > 0 && token == bestStarToken)) {
                continue;
            }

            // Mark as unknown waypoint if not an airway
            bool isAirway = NavdataObject::GetAirwayNetwork()->airwayExists(token);
            if (!isAirway) {
                parsedRoute.errors.push_back(
                    ParsingError{ UNKNOWN_WAYPOINT, "Unknown waypoint", i, token });
            }
        }
    }

    // Remove any overridden tokens from the raw route
    for (const auto& tokenToRemove : tokensToRemove) {
        std::string& rawRoute = parsedRoute.rawRoute;
        size_t pos = rawRoute.find(tokenToRemove);
        if (pos != std::string::npos) {
            // Remove the token and any adjacent space
            if (pos > 0 && rawRoute[pos - 1] == ' ') {
                // There's a space before the token
                rawRoute.erase(pos - 1, tokenToRemove.length() + 1);
            }
            else if (pos + tokenToRemove.length() < rawRoute.length() &&
                rawRoute[pos + tokenToRemove.length()] == ' ') {
                // There's a space after the token
                rawRoute.erase(pos, tokenToRemove.length() + 1);
            }
            else {
                // No adjacent space
                rawRoute.erase(pos, tokenToRemove.length());
            }
        }

        // Clean up any double spaces
        while (rawRoute.find("  ") != std::string::npos) {
            rawRoute.replace(rawRoute.find("  "), 2, " ");
        }

        // Trim leading/trailing spaces
        if (!rawRoute.empty() && rawRoute.front() == ' ') {
            rawRoute.erase(0, 1);
        }
        if (!rawRoute.empty() && rawRoute.back() == ' ') {
            rawRoute.pop_back();
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
    }
    else if (token == "VFR") {
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
        const Waypoint waypoint = Waypoint{ LATLON, token, point, LATLON };

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

            ParsedRouteSegment segment{ prevRouteWaypoint, routeWaypoint, "DCT",
                heading, // Set the heading
                -1 };
            parsedRoute.segments.push_back(segment);
        }

        parsedRoute.waypoints.push_back(routeWaypoint);
        previousWaypoint = waypoint; // Update the previous waypoint
        return true;
    }
    catch (const std::exception& e) {
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
            ParsedRouteSegment routeSegment{ fromWaypoint, toWaypoint, token,
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

        ParsedRouteSegment routeSegment{ fromWaypoint, toWaypoint, "DCT", heading, -1 };
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

    ParsedRouteSegment segment{ fromWaypoint, toWaypoint, "DCT", heading, -1 };
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
                }
                else {
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
                    }
                    else {
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