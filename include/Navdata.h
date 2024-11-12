#pragma once
#include "AirwayParser.h"
#include "Log.h"
#include "types/AirwayNetwork.h"
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
    SetWaypoints(std::unordered_multimap<std::string, Waypoint> newWaypoints,
                 std::unordered_multimap<std::string, Procedure> newProcedures)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      waypoints = newWaypoints;
      Log::info("Loaded {} waypoints into NavdataObject", waypoints.size());

      procedures = newProcedures;
      Log::info("Loaded {} procedures into NavdataObject", procedures.size());
    }

    static void LoadAirwayNetwork(std::string airwaysFilePath)
    {
      try
      {
        if (!std::filesystem::exists(airwaysFilePath))
        {
          Log::error("Airways file does not exist: {}", airwaysFilePath);
          return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        std::ifstream file(airwaysFilePath);
        if (!file.is_open())
        {
          Log::error("Failed to open airways file: {}", airwaysFilePath);
          return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        const auto parsed = AirwayParser::ParseAirwayTxt(buffer.str());
        if (!parsed)
        {
          Log::error("Error parsing airways file: {}", airwaysFilePath);
          return;
        }

        airwayNetwork = parsed.value();
      }
      catch (std::exception &e)
      {
        Log::error("Error loading airways: {}", e.what());
      }
    }

    static std::unordered_multimap<std::string, Waypoint> GetWaypoints()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return waypoints;
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

    static AirwayNetwork GetAirwayNetwork()
    {
      std::lock_guard<std::mutex> lock(_mutex);
      return airwayNetwork;
    }

    static Waypoint FindOrCreateWaypointByID(std::string identifier,
                                             erkir::spherical::Point position)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto it = waypoints.find(identifier);
      if (it != waypoints.end())
      {
        return it->second;
      }

      Waypoint waypoint(Utils::GetWaypointTypeByIdentifier(identifier), identifier, position);
      waypoints.emplace(identifier, waypoint);
      return waypoint;
    }

  private:
    inline static std::mutex _mutex;
    inline static std::unordered_multimap<std::string, Waypoint> waypoints = {};
    inline static std::unordered_multimap<std::string, Procedure> procedures = {};
    inline static AirwayNetwork airwayNetwork{};
  };

  // const static auto NavdataContainer = std::make_shared<NavdataObject>();
} // namespace RouteParser