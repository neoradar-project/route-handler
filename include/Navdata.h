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

  /**
   * @brief Sets the waypoints and procedures.
   * @param waypoints The waypoints to set.
   * @param procedures The procedures to set.
   */
  void SetWaypoints(std::multimap<std::string, Waypoint> waypoints,
                    std::multimap<std::string, Procedure> procedures) {
    this->waypoints = waypoints;
    Log::info("Loaded {} waypoints into NavdataObject", waypoints.size());

    this->procedures = procedures;
    Log::info("Loaded {} procedures into NavdataObject", procedures.size());
  }

  std::multimap<std::string, Waypoint> GetWaypoints() { return waypoints; }
  std::multimap<std::string, Procedure> GetProcedures() { return procedures; }

  /**
   * @brief Finds an airport by its ICAO code.
   * @param icao The ICAO code of the airport.
   * @return An optional containing the airport if found, or an empty optional.
   */
  std::optional<Waypoint> FindWaypointByType(std::string icao, WaypointType type);

  /**
   * @brief Finds the closest waypoint to a given waypoint by distance.
   * @param nextWaypoint The waypoint to find the closest to.
   * @param reference The reference waypoint.
   * @return An optional containing the closest waypoint if found, or an empty
   * optional.
   */
  std::optional<Waypoint>
  FindClosestWaypointTo(std::string nextWaypoint,
                        std::optional<Waypoint> reference);

  /**
   * @brief Finds a waypoint by its identifier.
   * @param identifier The identifier of the waypoint.
   * @return An optional containing the waypoint if found, or an empty optional.
   */
  std::optional<Waypoint> FindWaypoint(std::string identifier);

private:
  std::multimap<std::string, Waypoint> waypoints;
  std::multimap<std::string, Procedure> procedures;
};

const static auto NavdataContainer = std::make_shared<NavdataObject>();
} // namespace RouteParser