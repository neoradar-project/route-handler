#ifndef UTILS_H
#define UTILS_H

#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "types/ParsingError.h"
#include "types/RouteWaypoint.h"
#include <optional>
#include <string>

namespace RouteParser
{

  namespace Utils
  {
    static std::string CleanupRawRoute(std::string route)
    {
      route = absl::StrReplaceAll(route, {{":", " "}, {",", " "}});
      route = absl::StripAsciiWhitespace(route);
      return route;
    }

    static void InsertWaypointsAsRouteWaypoints(
        std::vector<RouteParser::RouteWaypoint> &waypoints,
        const std::vector<RouteParser::Waypoint> &parsedWaypoints,
        FlightRule currentFlightRule)
    {
      for (const auto &waypoint : parsedWaypoints)
      {
        waypoints.push_back(RouteParser::RouteWaypoint(
            waypoint.getType(), waypoint.getIdentifier(), waypoint.getPosition(),
            waypoint.getFrequencyHz(), currentFlightRule));
      }
    }

    static void SplitAirwayFields(std::string_view line, std::vector<std::string_view> &fields)
    {
      fields.clear();

      // Skip leading whitespace
      size_t start = 0;
      while (start < line.length() && std::isspace(line[start]))
      {
        ++start;
      }

      // Parse each field
      size_t pos = start;
      size_t length = line.length();

      while (pos < length)
      {
        // Skip whitespace
        while (pos < length && std::isspace(line[pos]))
        {
          ++pos;
        }

        if (pos >= length)
          break;

        // Find end of field
        size_t field_start = pos;
        while (pos < length && !std::isspace(line[pos]))
        {
          ++pos;
        }

        // Add field to vector
        fields.push_back(line.substr(field_start, pos - field_start));
      }
    }

    static RouteParser::RouteWaypoint WaypointToRouteWaypoint(
        RouteParser::Waypoint waypoint, FlightRule currentFlightRule = IFR,
        std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedPosition =
            std::nullopt)
    {
      return RouteParser::RouteWaypoint(
          waypoint.getType(), waypoint.getIdentifier(), waypoint.getPosition(),
          waypoint.getFrequencyHz(), currentFlightRule, plannedPosition);
    }

    static void
    InsertParsingErrorIfNotDuplicate(std::vector<ParsingError> &parsingErrors,
                                     const ParsingError &error)
    {
      auto it =
          std::find_if(parsingErrors.begin(), parsingErrors.end(),
                       [&error](const ParsingError &existingError)
                       {
                         return existingError.level == error.level &&
                                existingError.message == error.message &&
                                existingError.tokenIndex == error.tokenIndex &&
                                existingError.token == error.token &&
                                existingError.type == error.type;
                       });
      if (it == parsingErrors.end())
      {
        parsingErrors.push_back(error);
      }
    }
  } // namespace Utils
} // namespace RouteParser
#endif // UTILS_H