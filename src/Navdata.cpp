#include "Navdata.h"

using namespace RouteParser;

NavdataObject::NavdataObject()
{
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
    return waypointsByDistance.begin()->second;
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
