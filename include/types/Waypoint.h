#pragma once
#include "sphericalpoint.h"
#include <string>

namespace RouteParser {

enum WaypointType { FIX, VOR, DME, TACAN, NDB, AIRPORT };

class Waypoint {
public:
  Waypoint();
  Waypoint(WaypointType type, std::string identifier,
           erkir::spherical::Point position, int frequencyHz = 0) {
    this->type = type;
    this->identifier = identifier;
    this->position = position;
    this->frequencyHz = frequencyHz;
  }
  WaypointType getType() { return type; };
  std::string getIdentifier() { return identifier; };
  erkir::spherical::Point getPosition() { return position; };
  int getFrequencyHz() { return frequencyHz; };

  double distanceToInMeters(Waypoint other) {
    return this->position.distanceTo(other.getPosition());
  }

private:
  WaypointType type;
  std::string identifier;
  int frequencyHz;
  erkir::spherical::Point position;
};
} // namespace RouteParser