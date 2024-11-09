#include "Parser.h"
#include "Navdata.h"
#include "SidStarParser.h"
#include "Utils.h"
#include "types/ParsedRoute.h"

using namespace RouteParser;

bool ParserHandler::ParseFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                                          std::string token,
                                          std::string anchorIcao, bool strict) {

  auto res = SidStarParser::FindProcedure(token, anchorIcao,
                                          index == 0 ? SID : STAR, index);

  if (res.errors.size() > 0) {
    for (const auto &error : res.errors) {
      // This function is called twice, once in strict mode and once in non strict mode
      // We don't want to add the same error twice
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
    parsedRoute.SID = strict && res.extractedProcedure
                          ? res.extractedProcedure->name
                          : res.procedure;
  } else {
    parsedRoute.arrivalRunway = res.runway;
    parsedRoute.STAR = strict && res.extractedProcedure
                           ? res.extractedProcedure->name
                           : res.procedure;
  }

  if (res.extractedProcedure) {
    Utils::InsertWaypointsAsRouteWaypoints(parsedRoute.waypoints,
                                           res.extractedProcedure->waypoints);
    return true; // Parsed a procedure
  }

  return !strict && (res.procedure || res.runway);
}

bool ParserHandler::ParseWaypoints(ParsedRoute &parsedRoute, int index,
                                   std::string token,
                                   std::optional<Waypoint> &previousWaypoint) {
  auto waypoint =
      NavdataContainer->FindClosestWaypointTo(token, previousWaypoint);
  if (waypoint.has_value()) {
    parsedRoute.waypoints.push_back(
        Utils::WaypointToRouteWaypoint(waypoint.value()));
    previousWaypoint = waypoint;
    return true;
  }

  return false; // Not a waypoint
}

ParsedRoute ParserHandler::ParseRawRoute(std::string route, std::string origin,
                                         std::string destination) {
  auto parsedRoute = ParsedRoute();
  parsedRoute.rawRoute = route;

  route = Utils::CleanupRawRoute(route);

  if (route.empty()) {
    parsedRoute.errors.push_back({ROUTE_EMPTY, "Route is empty", 0, "", ERROR});
    return parsedRoute;
  }

  const std::vector<std::string> routeParts = absl::StrSplit(route, ' ');
  parsedRoute.totalTokens = routeParts.size();

  auto previousWaypoint = NavdataContainer->FindWaypointByType(origin, AIRPORT);

  for (auto i = 0; i < routeParts.size(); i++) {
    const auto token = routeParts[i];

    if (token.empty() || token == origin || token == destination ||
        token == " ") {
      parsedRoute.totalTokens--;
      continue;
    }

    if (token == "DCT" || token == "." || token == "..") {
      continue; // These count as tokens but we don't need to do anything with
                // them
    }

    if ((i == 0 || i == routeParts.size() - 1) &&
        token.find("/") != std::string::npos) {

      // If we're in the first and last part and there is a '/' in the token,
      // we can assume it's a SID/STAR or runway, so we start with that, but
      // in strict mode, meaning we will only accept something we have in
      // dataset hence that we know for sure is a SID/STAR
      if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                      i == 0 ? origin : destination, true)) {
        continue;
      }
    }
    // We always first check if it's a waypoint directly
    if (this->ParseWaypoints(parsedRoute, i, token, previousWaypoint)) {
      continue; // Found a waypoint, so we can skip the rest
    }

    // We check if it's a LAT/LON coordinate

    // TODO: Check for airways

    if (i == 0 || i == routeParts.size() - 1) {
      // If we're in the first and last part and we didn't find a waypoint or
      // lat/lon, we can assume it's a SID/STAR or runway, but not in strict
      // mode, meaning we will accept something that looks like a procedure but
      // might not be one
      // We assume it's not an airway, because an airway cannot be at the first
      // or last part If it is, the flight plan is invalid
      if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                      i == 0 ? origin : destination, false)) {
        continue;
      }
    }

    // If we reach this point, we have an unknown token
    parsedRoute.errors.push_back(
        ParsingError{UNKNOWN_WAYPOINT, "Unknown waypoint", i, token});
  }

  return parsedRoute;
}