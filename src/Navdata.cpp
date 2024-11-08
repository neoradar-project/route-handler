#include "Navdata.h"

using namespace RouteParser;

std::optional<Waypoint> NavdataObject::FindWaypointByType(std::string icao, WaypointType type) {
  auto range = waypoints.equal_range(icao);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.getType() == type) {
      return it->second;
    }
  }
  return std::nullopt;
}

std::optional<Waypoint> RouteParser::NavdataObject::FindClosestWaypointTo(
    std::string nextWaypoint, std::optional<Waypoint> reference) {

  if (!reference.has_value()) {
    FindWaypoint(nextWaypoint);
  }

  std::multimap<double, Waypoint> waypointsByDistance;
  auto range = waypoints.equal_range(nextWaypoint);
  for (auto it = range.first; it != range.second; ++it) {
    waypointsByDistance.insert(
        {reference->distanceToInMeters(it->second), it->second});
  }

  if (waypointsByDistance.size() > 0) {
    // Multimaps are always sorted by key automatically
    return waypointsByDistance.begin()->second;
  }

  return std::nullopt;
}

std::optional<Waypoint>
RouteParser::NavdataObject::FindWaypoint(std::string identifier) {
  auto range = waypoints.equal_range(identifier);
  for (auto it = range.first; it != range.second; ++it) {
    return it->second;
  }
  return std::nullopt;
}
