#pragma once
#include "Units.h"
#include "Waypoint.h"
#include <optional>
#include <string>

namespace RouteParser {

class RouteWaypoint : public Waypoint {

public:
  struct PlannedPosition {
    std::optional<int> plannedAltitude = std::nullopt;
    std::optional<int> plannedSpeed = std::nullopt;
    std::optional<int> plannedMach = std::nullopt;
    Units::Distance altitudeUnit = Units::Distance::FEET;
    Units::Speed speedUnit = Units::Speed::KNOTS;
  };

  RouteWaypoint(WaypointType type, std::string identifier,
                erkir::spherical::Point position, int frequencyHz = 0,
                std::optional<PlannedPosition> plannedPosition = std::nullopt)
      : Waypoint(type, identifier, position, frequencyHz) {
    this->plannedPosition = plannedPosition;
  }

  std::optional<PlannedPosition> GetPlannedPosition() const {
    return plannedPosition;
  }

private:
  std::optional<PlannedPosition> plannedPosition = std::nullopt;
};
}; // namespace RouteParser