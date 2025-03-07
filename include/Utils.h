#ifndef UTILS_H
#define UTILS_H

#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_join.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "types/ParsingError.h"
#include "types/RouteWaypoint.h"
#include <optional>
#include <string>
#include "Regexes.h"
#include <ctre.hpp>
namespace RouteParser
{
  namespace Utils
  {
    static std::string CleanupRawRoute(std::string route)
    {
      // First replace colons and commas with spaces and remove the + sign when amended
      route = absl::StrReplaceAll(route, {{":", " "}, {",", " "}, {"+", ""}});

      // Strip leading/trailing whitespace
      route = absl::StripAsciiWhitespace(route);

      // Split into tokens and rejoin with single spaces
      std::vector<std::string> tokens = absl::StrSplit(route, absl::ByAnyChar(" \t"), absl::SkipEmpty());
      return absl::StrJoin(tokens, " ");
    }

    // Rest of the Utils namespace remains the same...
    static void InsertWaypointsAsRouteWaypoints(
        std::vector<RouteParser::RouteWaypoint> &waypoints,
        const std::vector<RouteParser::Waypoint> &parsedWaypoints,
        FlightRule currentFlightRule)
    {
      for (const auto &waypoint : parsedWaypoints)
      {
        auto newPos = erkir::spherical::Point(waypoint.getPosition().latitude(), waypoint.getPosition().longitude());
        waypoints.push_back(RouteParser::RouteWaypoint(
            waypoint.getType(), waypoint.getIdentifier(), newPos,
            waypoint.getFrequencyHz(), currentFlightRule));
      }
    }

    static std::optional<erkir::spherical::Point> ParseDataFilePoint(
        std::string_view lat,
        std::string_view lng)
    {
      double latitude, longitude;

      try
      {
        latitude = std::stod(std::string(lat));
        longitude = std::stod(std::string(lng));
      }
      catch (const std::invalid_argument &)
      {
        return std::nullopt;
      }
      catch (const std::out_of_range &)
      {
        return std::nullopt;
      }

      return erkir::spherical::Point(latitude, longitude);
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
      auto newPos = erkir::spherical::Point(waypoint.getPosition().latitude(), waypoint.getPosition().longitude());
      return RouteParser::RouteWaypoint(
          waypoint.getType(), waypoint.getIdentifier(), newPos,
          waypoint.getFrequencyHz(), currentFlightRule, plannedPosition);
    }

    static void InsertParsingErrorIfNotDuplicate(
        std::vector<ParsingError> &parsingErrors,
        const ParsingError &error)
    {
      auto it = std::find_if(parsingErrors.begin(), parsingErrors.end(),
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

    static WaypointType GetWaypointTypeByIdentifier(std::string identifier)
    {
      if (ctre::match<RouteParser::Regexes::RouteVOR>(identifier))
      {
        return WaypointType::VOR;
      }

      if (ctre::match<RouteParser::Regexes::RouteNDB>(identifier))
      {
        return WaypointType::NDB;
      }
      if (ctre::match<RouteParser::Regexes::RouteFIX>(identifier))
      {
        return WaypointType::FIX;
      }

      return WaypointType::UNKNOWN;
    }

    static WaypointType GetWaypointTypeByTypeString(std::string type)
    {
      if (type == "VOR")
      {
        return WaypointType::VOR;
      }
      if (type == "NDB")
      {
        return WaypointType::NDB;
      }
      if (type == "FIX")
      {
        return WaypointType::FIX;
      }
      if (type == "NDB-DME")
      {
        return WaypointType::NDBDME;
      }
      if (type == "VOR-DME")
      {
        return WaypointType::VORDME;
      }
      if (type == "VORTAC")
      {
        return WaypointType::VORTAC;
      }
      return WaypointType::UNKNOWN;
    }
  } // namespace Utils
} // namespace RouteParser
#endif // UTILS_H