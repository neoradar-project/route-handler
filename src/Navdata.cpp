#include "Navdata.h"
#include <map>
#include <mio/mmap.hpp>

using namespace RouteParser;

NavdataObject::NavdataObject()
{
}

void NavdataObject::LoadAirwayNetwork(std::string airwaysFilePath)
{
  try
  {
    if (!std::filesystem::exists(airwaysFilePath))
    {
      Log::error("Airways file does not exist: {}", airwaysFilePath);
      return;
    }

    // Memory map the file
    mio::mmap_source mmap(airwaysFilePath);
    std::string_view content(mmap.data(), mmap.size());

    const auto parsed = AirwayParser::ParseAirwayTxt(content);

    if (!parsed)
    {
      std::cout << "Failed to parse airways" << std::endl;
      Log::error("Error parsing airways file: {}", airwaysFilePath);
      return;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    airwayNetwork = parsed.value();
  }
  catch (std::exception &e)
  {
    Log::error("Error loading airways: {}", e.what());
  }
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
    const char *fileData = mmap.data();
    size_t fileSize = mmap.size();

    // Reserve space in waypoints map to avoid rehashing
    constexpr size_t ESTIMATED_WAYPOINTS = 10000;
    std::lock_guard<std::mutex> lock(_mutex);
    size_t currentSize = waypoints.size();
    waypoints.reserve(currentSize + ESTIMATED_WAYPOINTS);

    std::string_view fileView(fileData, fileSize);
    std::vector<std::string_view> fields;
    fields.reserve(5); // Preallocate fields vector

    // Process file line by line without loading entire file
    size_t start = 0;
    size_t end = 0;
    while ((end = fileView.find('\n', start)) != std::string_view::npos)
    {
      std::string_view line = fileView.substr(start, end - start);
      start = end + 1;

      if (line.empty() || line[0] == ';')
        continue;

      fields.clear();
      size_t tab = 0;
      size_t fieldStart = 0;

      // Manual tab splitting (faster than StrSplit)
      while ((tab = line.find('\t', fieldStart)) != std::string_view::npos && fields.size() < 3)
      {
        fields.push_back(line.substr(fieldStart, tab - fieldStart));
        fieldStart = tab + 1;
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
  if (!reference.has_value())
  {
    reference = FindWaypoint(nextWaypoint);
    if (!reference.has_value())
    {
      return std::nullopt;
    }
  }

  std::multimap<double, Waypoint> waypointsByDistance;
  auto range = waypoints.equal_range(nextWaypoint);
  for (auto it = range.first; it != range.second; ++it)
  {
    waypointsByDistance.insert(
        {reference->distanceToInMeters(it->second), it->second});
  }

  if (!waypointsByDistance.empty())
  {
    return waypointsByDistance.begin()->second; // Maps are ordered by default
  }

  return std::nullopt;
}

std::optional<Waypoint>
RouteParser::NavdataObject::FindWaypoint(std::string identifier)
{
  auto waypts = GetWaypoints(); // Get a local copy
  auto range = waypts.equal_range(identifier);
  if (range.first != range.second) // Compare iterators from the same container
  {
    return range.first->second;
  }
  return std::nullopt;
}
