#pragma once
#include "Log.h"
#include "types/Procedure.h"
#include "types/Waypoint.h"
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include "Utils.h"
#include <iostream>
#include "AirwayNetwork.h"
#include "WaypointNetwork.h"
namespace RouteParser
{

  class NavdataObject
  {
  public:
    NavdataObject();

    /**
     * @brief Sets the waypoints and procedures.
     * @param waypoints The waypoints to set.
     * @param procedures The procedures to set.
     */
    static void
    SetProcedures(
        std::unordered_multimap<std::string, Procedure> newProcedures)
    {
      std::lock_guard<std::mutex> lock(_mutex);

      procedures = newProcedures;
      Log::info("Loaded {} procedures into NavdataObject", procedures.size());
    }
    static void LoadAirwayNetwork(std::string airwaysFilePath);

    static void LoadWaypoints(std::string waypointsFilePath);

    static void LoadIntersectionWaypoints(std::string isecFilePath);

    static const std::unordered_map<std::string, Waypoint> GetWaypoints()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return waypoints;
    }

    static const std::shared_ptr<WaypointNetwork> GetWaypointNetwork()
    {
      return waypointNetwork;
    }

    static void Reset()
    {
      if (waypointNetwork)
      {
        waypointNetwork = std::make_shared<WaypointNetwork>();
      }
      procedures.clear();
    }

    static std::unordered_multimap<std::string, Procedure> GetProcedures()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return procedures;
    }

    /**
     * @brief Finds an airport by its ICAO code.
     * @param icao The ICAO code of the airport.
     * @return An optional containing the airport if found, or an empty optional.
     */
    static std::optional<Waypoint> FindWaypointByType(std::string icao,
                                                      WaypointType type);

    /**
     * @brief Finds the closest waypoint to a given waypoint by distance.
     * @param nextWaypoint The waypoint to find the closest to.
     * @param reference The reference waypoint.
     * @return An optional containing the closest waypoint if found, or an empty
     * optional.
     */
    static std::optional<Waypoint>
    FindClosestWaypointTo(std::string nextWaypoint,
                          std::optional<Waypoint> reference);

    /**
     * @brief Finds a waypoint by its identifier.
     * @param identifier The identifier of the waypoint.
     * @return An optional containing the waypoint if found, or an empty optional.
     */
    static std::optional<Waypoint> FindWaypoint(std::string identifier);

    /**
     * @brief Finds the closest waypoint to a given point by distance.
     * @param identifier The identifier of the waypoint.
     * @param referencePoint The reference point.
     * @return An optional containing the closest waypoint if found, or an empty
     * optional.
     */
    static std::optional<Waypoint> FindClosestWaypoint(std::string identifier,
                                                       erkir::spherical::Point referencePoint);

    static std::shared_ptr<AirwayNetwork> GetAirwayNetwork()
    {
      return airwayNetwork;
    }

    static Waypoint FindOrCreateWaypointByID(std::string_view identifier,
                                             erkir::spherical::Point position)
    {
      std::lock_guard<std::mutex> lock(waypointsMutex);
      auto it = waypoints.find(std::string(identifier));
      if (it != waypoints.end())
      {
        return it->second;
      }

      auto [newIt, inserted] = waypoints.try_emplace(std::string(identifier),
                                                     Utils::GetWaypointTypeByIdentifier(std::string(identifier)),
                                                     std::string(identifier),
                                                     position);
      return newIt->second;
    }

  private:
    inline static std::mutex _mutex;
    inline static std::mutex waypointsMutex;

    static void BatchInsertWaypoints(std::vector<std::pair<std::string, Waypoint>> &&batch);

    inline static std::unordered_map<std::string, Waypoint> waypoints = {};
    inline static std::unordered_multimap<std::string, Procedure> procedures = {};
    inline static std::shared_ptr<AirwayNetwork> airwayNetwork;
    inline static std::shared_ptr<WaypointNetwork> waypointNetwork;
  };

  // const static auto NavdataContainer = std::make_shared<NavdataObject>();
} // namespace RouteParser