#include "Navdata.h"
#include <map>
#include <mio/mmap.hpp>
#include <memory>
#include <string>

using namespace RouteParser;

NavdataObject::NavdataObject()
{
  waypointNetwork = std::make_shared<WaypointNetwork>();
}

void NavdataObject::LoadAirwayNetwork(std::string airwaysFilePath)
{
  if (!waypointNetwork)
  {
    waypointNetwork = std::make_shared<WaypointNetwork>();
  }
  waypointNetwork->addProvider(std::make_unique<AirwayWaypointProvider>(
      airwaysFilePath, "Airways DB"));
  airwayNetwork = std::make_shared<AirwayNetwork>(airwaysFilePath);
}

void NavdataObject::LoadWaypoints(std::string waypointsFilePath)
{

  if (waypointsFilePath.empty() || !std::filesystem::exists(waypointsFilePath))
  {
    Log::error("Waypoints file does not exist: {}", waypointsFilePath);
    return;
  }

  if (!waypointNetwork)
  {
    waypointNetwork = std::make_shared<WaypointNetwork>();
  }
  waypointNetwork->addProvider(std::make_unique<NavdataWaypointProvider>(
      waypointsFilePath, "Waypoints DB"));
}

void NavdataObject::LoadIntersectionWaypoints(std::string isecFilePath)
{
  try
  {
    if (!std::filesystem::exists(isecFilePath))
    {
      Log::error("Intersection waypoints file does not exist: {}", isecFilePath);
      return;
    }

    // Memory map the file
    mio::mmap_source mmap(isecFilePath);
    const char *data = mmap.data();
    const char *end = data + mmap.size();

    // Pre-allocate vector for batch insertion
    std::vector<std::pair<std::string, Waypoint>> batch_waypoints;
    batch_waypoints.reserve(300000); // Increased reservation based on file size

    const char *line_start = data;
    const char *line_end;

    while (line_start < end && (line_end = static_cast<const char *>(memchr(line_start, '\n', end - line_start))))
    {
      if (*line_start == ';' || line_start == line_end)
      {
        line_start = line_end + 1;
        continue;
      }

      // Fast field extraction using pointers
      const char *tab1 = static_cast<const char *>(memchr(line_start, '\t', line_end - line_start));
      if (!tab1)
        continue;

      const char *tab2 = static_cast<const char *>(memchr(tab1 + 1, '\t', line_end - (tab1 + 1)));
      if (!tab2)
        continue;

      // Extract identifier
      std::string_view identifier(line_start, tab1 - line_start);

      // Fast number parsing using std::stod
      double lat, lon;
      const char *num_start = tab1 + 1;
      const char *num_end = tab2;
      try
      {
        lat = std::stod(std::string(num_start, num_end));
      }
      catch (const std::invalid_argument &)
      {
        continue;
      }
      catch (const std::out_of_range &)
      {
        continue;
      }

      num_start = tab2 + 1;
      num_end = line_end;
      try
      {
        lon = std::stod(std::string(num_start, num_end));
      }
      catch (const std::invalid_argument &)
      {
        continue;
      }
      catch (const std::out_of_range &)
      {
        continue;
      }

      erkir::spherical::Point point(lat, lon);
      batch_waypoints.emplace_back(
          std::string(identifier), // First part of the pair (string)
          Waypoint(                // Second part of the pair (Waypoint)
              Utils::GetWaypointTypeByIdentifier(std::string(identifier)),
              std::string(identifier),
              point));

      line_start = line_end + 1;
    }

    // Single locked batch insertion
    std::lock_guard<std::mutex> lock(_mutex);
    BatchInsertWaypoints(std::move(batch_waypoints));
  }
  catch (const std::exception &e)
  {
    Log::error("Error loading intersection waypoints: {}", e.what());
  }
}

void NavdataObject::BatchInsertWaypoints(std::vector<std::pair<std::string, Waypoint>> &&batch)
{
  waypoints.reserve(waypoints.size() + batch.size());
  for (auto &&pair : batch)
  {
    waypoints.emplace(std::move(pair.first), std::move(pair.second));
  }
}

std::optional<Waypoint> NavdataObject::FindWaypointByType(std::string icao,
                                                          WaypointType type)
{
  auto range = waypoints.equal_range(icao);
  for (auto it = range.first; it != range.second; ++it)
  {
    if (it->second.getType() == type)
    {
      return it->second;
    }
  }
  return std::nullopt;
}

std::optional<Waypoint> RouteParser::NavdataObject::FindClosestWaypointTo(
    std::string nextWaypoint, std::optional<Waypoint> reference)
{
  return waypointNetwork->findClosestWaypointTo(nextWaypoint, reference);
}

std::optional<Waypoint>
RouteParser::NavdataObject::FindWaypoint(std::string identifier)
{
  return waypointNetwork->findFirstWaypoint(identifier);
}

std::optional<Waypoint> NavdataObject::FindClosestWaypoint(std::string identifier, erkir::spherical::Point referencePoint)
{
  return waypointNetwork->findClosestWaypoint(identifier, referencePoint);
}
