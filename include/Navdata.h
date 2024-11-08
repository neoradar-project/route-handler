#pragma once
#include "Log.h"
#include "types/Procedure.h"
#include "types/Waypoint.h"
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace RouteParser {

class NavdataObject {
public:
  NavdataObject();

  void SetWaypoints(std::multimap<std::string, Waypoint> waypoints,
                    std::multimap<std::string, Procedure> procedures) {
    this->waypoints = waypoints;
    Log::info("Loaded {} waypoints into NavdataObject", waypoints.size());

    this->procedures = procedures;
    Log::info("Loaded {} procedures into NavdataObject", procedures.size());
  }

  std::multimap<std::string, Waypoint> GetWaypoints() { return waypoints; }
  std::multimap<std::string, Procedure> GetProcedures() { return procedures; }

  std::optional<Waypoint> FindAirport(std::string icao) {
    auto range = waypoints.equal_range(icao);
    for (auto it = range.first; it != range.second; ++it) {
      if (it->second.getType() == WaypointType::AIRPORT) {
        return it->second;
      }
    }
    return std::nullopt;
  }

  std::optional<Waypoint>
  FindClosestWaypointTo(std::string nextWaypoint,
                        std::optional<Waypoint> reference) {

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

  std::optional<Waypoint> FindWaypoint(std::string identifier) {
    auto range = waypoints.equal_range(identifier);
    for (auto it = range.first; it != range.second; ++it) {
      return it->second;
    }
    return std::nullopt;
  }

private:
  std::multimap<std::string, Waypoint> waypoints;
  std::multimap<std::string, Procedure> procedures;
};

const static auto NavdataContainer = std::make_shared<NavdataObject>();
} // namespace RouteParser