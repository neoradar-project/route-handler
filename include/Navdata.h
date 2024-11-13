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
#include <iostream>
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
          std::cout << "Failed to parse airways" << std::endl;
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

    static void LoadIntersectionWaypoints(std::string isecFilePath)
    {
      try
      {
        if (!std::filesystem::exists(isecFilePath))
        {
          Log::error("Intersection waypoints file does not exist: {}", isecFilePath);
          return;
        }

        // Pre-allocate a large buffer for reading
        constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer
        std::vector<char> buffer(BUFFER_SIZE);

        // Read file in chunks directly into waypoints
        std::ifstream file(isecFilePath, std::ios::binary);
        if (!file)
        {
          Log::error("Failed to open intersection waypoints file: {}", isecFilePath);
          return;
        }

        // Reserve space in waypoints map to avoid rehashing
        constexpr size_t ESTIMATED_WAYPOINTS = 10000;
        std::lock_guard<std::mutex> lock(_mutex);
        size_t currentSize = waypoints.size();
        waypoints.reserve(currentSize + ESTIMATED_WAYPOINTS);

        std::string line;
        line.reserve(256); // Preallocate line buffer
        std::vector<std::string_view> fields;
        fields.reserve(5); // Preallocate fields vector

        // Process file line by line without loading entire file
        while (std::getline(file, line))
        {
          if (line.empty() || line[0] == ';')
            continue;

          fields.clear();
          size_t start = 0;
          size_t tab = 0;

          // Manual tab splitting (faster than StrSplit)
          while ((tab = line.find('\t', start)) != std::string::npos && fields.size() < 3)
          {
            fields.push_back(std::string_view(line.data() + start, tab - start));
            start = tab + 1;
          }
          if (fields.size() < 3)
            continue;

          // Fast string to double conversion
          char *endPtr;
          double lat = std::strtod(std::string(fields[1]).c_str(), &endPtr);
          if (*endPtr != '\0')
            continue;

          double lon = std::strtod(std::string(fields[2]).c_str(), &endPtr);
          if (*endPtr != '\0')
            continue;

          // Create waypoint and insert directly
          erkir::spherical::Point point(lat, lon);
          std::string identifier(fields[0]);

          // Only create if it doesn't exist
          NavdataObject::FindOrCreateWaypointByID(identifier, point);
        }
      }
      catch (const std::exception &e)
      {
        Log::error("Error loading intersection waypoints: {}", e.what());
      }
    }

    static const std::unordered_multimap<std::string, Waypoint> GetWaypoints()
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

    /* Used for testing */

    static AirwayNetwork SetAirwayNetwork(AirwayNetwork network)
    {
      std::lock_guard<std::mutex> lock(_mutex);
      airwayNetwork = network;
      return airwayNetwork;
    }

    static Waypoint FindOrCreateWaypointByID(std::string identifier,
                                             erkir::spherical::Point position)
    {
      std::lock_guard<std::mutex> lock(waypointsMutex);
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
    inline static std::mutex waypointsMutex;

    inline static std::unordered_multimap<std::string, Waypoint> waypoints = {};
    inline static std::unordered_multimap<std::string, Procedure> procedures = {};
    inline static AirwayNetwork airwayNetwork{};
  };

  // const static auto NavdataContainer = std::make_shared<NavdataObject>();
} // namespace RouteParser