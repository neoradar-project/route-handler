#include "Parser.h"
#include "Log.h"
#include "Navdata.h"
#include "SidStarParser.h"
#include "Utils.h"
#include "absl/strings/str_split.h"
#include "erkir/geo/sphericalpoint.h"
#include "types/ParsedRoute.h"
#include "types/ParsingError.h"
#include "types/RouteWaypoint.h"
#include "types/Waypoint.h"
#include <optional>
#include <vector>

using namespace RouteParser;

bool ParserHandler::ParseFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                                          std::string token,
                                          std::string anchorIcao, bool strict,
                                          FlightRule currentFlightRule) {

  auto res = SidStarParser::FindProcedure(token, anchorIcao,
                                          index == 0 ? SID : STAR, index);

  if (res.errors.size() > 0) {
    for (const auto &error : res.errors) {
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
                                           res.extractedProcedure->waypoints,
                                           currentFlightRule);
    return true; // Parsed a procedure
  }

  return !strict && (res.procedure || res.runway);
}

bool ParserHandler::ParseWaypoints(ParsedRoute &parsedRoute, int index,
                                   std::string token,
                                   std::optional<Waypoint> &previousWaypoint,
                                   FlightRule currentFlightRule) {
  const std::vector<std::string> parts = absl::StrSplit(token, '/');
  std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd =
      std::nullopt;
  token = parts[0];

  auto waypoint = NavdataObject::FindClosestWaypointTo(token, previousWaypoint);
  if (waypoint) {
    if (parts.size() > 1) {
      plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
      if (!plannedAltAndSpd) {
        // Misformed second part of waypoint data
        parsedRoute.errors.push_back(
            {INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
             index, token + '/' + parts[1], ERROR});
      }
    }
    parsedRoute.waypoints.push_back(Utils::WaypointToRouteWaypoint(
        waypoint.value(), currentFlightRule, plannedAltAndSpd));
    previousWaypoint = waypoint;
    return true;
  }

  return false; // Not a waypoint
}

std::optional<RouteWaypoint::PlannedAltitudeAndSpeed>
ParserHandler::ParsePlannedAltitudeAndSpeed(int index, std::string rightToken) {
  // Example is WAYPOINT/N0490F370 (knots) or WAYPOINT/M083F360 (mach) or
  // WAYPOINT/K0880F360 (kmh) For alt F370 is FL feet, S0150 is 1500 meters,
  // A055 is alt 5500, M0610 is alt 6100 meters For speed, K0880 is 880 km/h,
  // M083 is mach 0.83, S0150 is 150 knots
  auto match = ctre::match<RouteParser::Regexes::RoutePlannedAltitudeAndSpeed>(
      rightToken);
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
      altitudeUnit =
          Units::Distance::METERS; // Todo: distinguish between FL and Alt
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

    return RouteWaypoint::PlannedAltitudeAndSpeed{
        extractedAltitude, extractedSpeed, altitudeUnit, speedUnit};
  } catch (const std::exception &e) {
    Log::error("Error trying to parse planned speed and altitude {}", e.what());
    return std::nullopt;
  }

  return std::nullopt;
}

ParsedRoute ParserHandler::ParseRawRoute(std::string route, std::string origin,
                                         std::string destination,
                                         FlightRule filedFlightRule) {
  auto parsedRoute = ParsedRoute();
  parsedRoute.rawRoute = route;

  route = Utils::CleanupRawRoute(route);

  if (route.empty()) {
    parsedRoute.errors.push_back({ROUTE_EMPTY, "Route is empty", 0, "", ERROR});
    return parsedRoute;
  }

  const std::vector<std::string> routeParts = absl::StrSplit(route, ' ');
  parsedRoute.totalTokens = routeParts.size();

  auto previousWaypoint = NavdataObject::FindWaypointByType(origin, AIRPORT);
  FlightRule currentFlightRule = filedFlightRule;

  for (auto i = 0; i < routeParts.size(); i++) {
    const auto token = routeParts[i];

    if (token.empty() || token == origin || token == destination ||
        token == " " || token == "." || token == "..") {
      parsedRoute.totalTokens--;
      continue;
    }

    if (token == "DCT") {
      continue; // These count as tokens but we don't need to do anything with
                // them
    }

    // This is a very simple check, so we can run it already
    if (this->ParseFlightRule(currentFlightRule, i, token)) {
      continue;
    }

    if (i == 0) {
      // Sometimes, the flightplan is prefixed with the TAS and planned level.
      // If that's the case, we'll ignore it as it should be specified elsewhere
      // in the flightplan
      // It still counts as a token
      if (this->ParsePlannedAltitudeAndSpeed(i, token)) {
        continue;
      }
    }

    if ((i == 0 || i == routeParts.size() - 1) &&
        token.find("/") != std::string::npos) {

      // If we're in the first and last part and there is a '/' in the token,
      // we can assume it's a SID/STAR or runway, so we start with that, but
      // in strict mode, meaning we will only accept something we have in
      // dataset hence that we know for sure is a SID/STAR
      if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                      i == 0 ? origin : destination, true,
                                      currentFlightRule)) {
        continue;
      }
    }
    // We always first check if it's a waypoint directly
    if (this->ParseWaypoints(parsedRoute, i, token, previousWaypoint,
                             currentFlightRule)) {
      continue; // Found a waypoint, so we can skip the rest
    }

    // We check if it's a LAT/LON coordinate
    if (this->ParseLatLon(parsedRoute, i, token, previousWaypoint,
                          currentFlightRule)) {

      continue;
    }

    // TODO: Check for airways

    if (i == 0 || i == routeParts.size() - 1) {
      // If we're in the first and last part and we didn't find a waypoint or
      // lat/lon, we can assume it's a SID/STAR or runway, but not in strict
      // mode, meaning we will accept something that looks like a procedure but
      // might not be in database
      // We assume it's not an airway, because an airway cannot be at the first
      // or last part If it is, the flight plan is invalid
      if (this->ParseFirstAndLastPart(parsedRoute, i, token,
                                      i == 0 ? origin : destination, false,
                                      currentFlightRule)) {
        continue;
      }
    }

    // If we reach this point, we have an unknown token
    parsedRoute.errors.push_back(
        ParsingError{UNKNOWN_WAYPOINT, "Unknown waypoint", i, token});
  }

  return parsedRoute;
}

bool RouteParser::ParserHandler::ParseFlightRule(FlightRule &currentFlightRule,
                                                 int index, std::string token) {
  if (token == "IFR") {
    currentFlightRule = IFR;
    return true;
  } else if (token == "VFR") {
    currentFlightRule = VFR;
    return true;
  }
  return false;
}

bool RouteParser::ParserHandler::ParseLatLon(
    ParsedRoute &parsedRoute, int index, std::string token,
    std::optional<Waypoint> &previousWaypoint, FlightRule currentFlightRule) {
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
      parsedRoute.errors.push_back(
          {INVALID_DATA, "Invalid lat/lon coordinate", index, token, ERROR});
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
    const Waypoint waypoint = Waypoint{LATLON, token, point, LATLON};

    std::optional<RouteWaypoint::PlannedAltitudeAndSpeed> plannedAltAndSpd =
        std::nullopt;
    if (parts.size() > 1) {
      plannedAltAndSpd = this->ParsePlannedAltitudeAndSpeed(index, parts[1]);
      if (!plannedAltAndSpd) {
        // Misformed second part of waypoint data
        parsedRoute.errors.push_back(
            {INVALID_DATA, "Invalid planned TAS and Altitude, ignoring it.",
             index, token + '/' + parts[1], ERROR});
      }
    }
    parsedRoute.waypoints.push_back(Utils::WaypointToRouteWaypoint(
        waypoint, currentFlightRule, plannedAltAndSpd));
    previousWaypoint = waypoint; // Update the previous waypoint
    return true;
  } catch (const std::exception &e) {
    parsedRoute.errors.push_back(
        {INVALID_DATA, "Invalid lat/lon coordinate", index, token, ERROR});
    Log::error("Error trying to parse lat/lon ({}): {}", token, e.what());
    return false;
  }

  return false;
};

bool RouteParser::ParserHandler::ParseAirway(
    ParsedRoute &parsedRoute, int index, std::string token,
    std::optional<Waypoint> &previousWaypoint,
    std::optional<std::string> nextToken, FlightRule currentFlightRule) {
  if (!nextToken) {
    return false; // We don't have a next waypoint, so we don't know where the
                  // airway ends
  }

  if (token.find('/') != std::string::npos) {
    return false; // Airway segments cannot have planned altitude and speed
  }

  if (!NavdataObject::GetAirwayNetwork().getAirway(token)) {
    return false; // Airway not found
  }

  // We need to check if the airway is valid
  auto airwaySegments = NavdataObject::GetAirwayNetwork().getFixesBetween(
      token, previousWaypoint->getIdentifier(), nextToken.value_or(""),
      previousWaypoint->getPosition());

  if (airwaySegments.empty()) {
    parsedRoute.errors.push_back({INVALID_AIRWAY_FORMAT,
                                  "Airway segment not found", index, token,
                                  ERROR});
    return false;
  }

  for (const auto &segment : airwaySegments) {
    // parsedRoute.waypoints.push_back(
    //     Utils::WaypointToRouteWaypoint(segment, currentFlightRule));
  }

  return true;
};