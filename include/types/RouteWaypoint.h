#pragma once
#include "Units.h"
#include "Waypoint.h"
#include <optional>
#include <string>

namespace RouteParser {

class RouteWaypoint : public Waypoint {

public:
  struct PlannedAltitudeAndSpeed {
    std::optional<int> plannedAltitude = std::nullopt;
    std::optional<int> plannedSpeed = std::nullopt;
    Units::Distance altitudeUnit = Units::Distance::FEET;
    Units::Speed speedUnit = Units::Speed::KNOTS;
  };

  RouteWaypoint(
      WaypointType type, std::string identifier,
      erkir::spherical::Point position, int frequencyHz = 0,
      FlightRule flightRule = IFR,
      std::optional<PlannedAltitudeAndSpeed> plannedPosition = std::nullopt)
      : Waypoint(type, identifier, position, frequencyHz) {
    this->plannedPosition = plannedPosition;
    this->flightRule = flightRule;
  }

  std::optional<PlannedAltitudeAndSpeed> GetPlannedPosition() const {
    return plannedPosition;
  }

private:
  std::optional<PlannedAltitudeAndSpeed> plannedPosition = std::nullopt;
  FlightRule flightRule = IFR;
};
}; // namespace RouteParser