#ifndef UTILS_H
#define UTILS_H

#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "types/ParsingError.h"
#include "types/RouteWaypoint.h"
#include <optional>
#include <string>

namespace RouteParser {

namespace Utils {
static std::string CleanupRawRoute(std::string route) {
  route = absl::StrReplaceAll(route, {{":", " "}, {",", " "}});
  route = absl::StripAsciiWhitespace(route);
  return route;
}

static void InsertWaypointsAsRouteWaypoints(
    std::vector<RouteParser::RouteWaypoint> &waypoints,
    const std::vector<RouteParser::Waypoint> &parsedWaypoints) {
  for (const auto &waypoint : parsedWaypoints) {
    waypoints.push_back(RouteParser::RouteWaypoint(
        waypoint.getType(), waypoint.getIdentifier(), waypoint.getPosition(),
        waypoint.getFrequencyHz()));
  }
}

static RouteParser::RouteWaypoint WaypointToRouteWaypoint(
    RouteParser::Waypoint waypoint,
    std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedPosition =
        std::nullopt) {
  return RouteParser::RouteWaypoint(
      waypoint.getType(), waypoint.getIdentifier(), waypoint.getPosition(),
      waypoint.getFrequencyHz(), plannedPosition);
}

static void
InsertParsingErrorIfNotDuplicate(std::vector<ParsingError> &parsingErrors,
                                 const ParsingError &error) {
  auto it =
      std::find_if(parsingErrors.begin(), parsingErrors.end(),
                   [&error](const ParsingError &existingError) {
                     return existingError.level == error.level &&
                            existingError.message == error.message &&
                            existingError.tokenIndex == error.tokenIndex &&
                            existingError.token == error.token &&
                            existingError.type == error.type;
                   });
  if (it == parsingErrors.end()) {
    parsingErrors.push_back(error);
  }
}
} // namespace Utils
} // namespace RouteParser
#endif // UTILS_H