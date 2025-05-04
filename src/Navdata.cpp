#include "Navdata.h"
#include <map>
#include <memory>
#include <mio/mmap.hpp>
#include <string>

using namespace RouteParser;

NavdataObject::NavdataObject() { waypointNetwork = std::make_shared<WaypointNetwork>(); }

void NavdataObject::LoadAirwayNetwork(std::string airwaysFilePath)
{
    if (!waypointNetwork)
    {
        waypointNetwork = std::make_shared<WaypointNetwork>();
    }
    waypointNetwork->addProvider(
        std::make_unique<AirwayWaypointProvider>(airwaysFilePath, "Airways DB"));
    airwayNetwork = std::make_shared<AirwayNetwork>(airwaysFilePath);
}

void NavdataObject::LoadWaypoints(std::string waypointsFilePath)
{

    if (waypointsFilePath.empty() || !std::filesystem::exists(waypointsFilePath))
    {
        Log::error("Waypoints file does not exist, unable to load it.");
        return;
    }

    if (!waypointNetwork)
    {
        waypointNetwork = std::make_shared<WaypointNetwork>();
    }

    waypointNetwork->addProvider(
        std::make_unique<NavdataWaypointProvider>(waypointsFilePath, "Waypoints DB"));
}

void NavdataObject::LoadAirports(std::string airportsFilePath)
{

    airportNetwork = std::make_shared<AirportNetwork>(airportsFilePath);
}

void NavdataObject::LoadRunways(std::string runwaysFilePath)
{

    runwayNetwork = std::make_shared<RunwayNetwork>(runwaysFilePath);
}

void NavdataObject::LoadNseWaypoints(
    const std::vector<Waypoint>& waypoints, const std::string& providerName)
{
    if (!waypointNetwork) {
        waypointNetwork = std::make_shared<WaypointNetwork>();
    }

    waypointNetwork->addProvider(
        std::make_unique<NseWaypointProvider>(waypoints, providerName));
}

std::optional<Waypoint> RouteParser::NavdataObject::FindWaypoint(std::string identifier)
{
    // Try waypoint network first
    auto waypoint = waypointNetwork->findFirstWaypoint(identifier);

    // If not found and identifier is 4 characters, try as airport
    if (!waypoint && identifier.length() == 4 && airportNetwork)
    {
        if (auto airport = airportNetwork->findAirport(identifier))
        {
            return airport->toWaypoint();
        }
    }

    return waypoint;
}

std::optional<Waypoint> NavdataObject::FindClosestWaypoint(
    std::string identifier, erkir::spherical::Point referencePoint)
{
    // Try waypoint network first
    auto waypoint = waypointNetwork->findClosestWaypoint(identifier, referencePoint);

    // If not found and identifier is 4 characters, try as airport
    if (!waypoint && identifier.length() == 4 && airportNetwork)
    {
        if (auto airport = airportNetwork->findAirport(identifier))
        {
            return airport->toWaypoint();
        }
    }

    return waypoint;
}

std::optional<Waypoint> RouteParser::NavdataObject::FindClosestWaypointTo(
    std::string nextWaypoint, std::optional<Waypoint> reference)
{
    // Try waypoint network first
    auto waypoint = waypointNetwork->findClosestWaypointTo(nextWaypoint, reference);

    // If not found and identifier is 4 characters, try as airport
    if (!waypoint && nextWaypoint.length() == 4 && airportNetwork)
    {
        if (auto airport = airportNetwork->findAirport(nextWaypoint))
        {
            return airport->toWaypoint();
        }
    }

    return waypoint;
}

std::optional<Waypoint> NavdataObject::FindWaypointByType(
    std::string icao, WaypointType type)
{
    // If specifically looking for AIRPORT type and identifier is 4 characters
    if (type == WaypointType::AIRPORT && icao.length() == 4 && airportNetwork)
    {
        if (auto airport = airportNetwork->findAirport(icao))
        {
            return airport->toWaypoint();
        }
    }

    // Otherwise check waypoints map
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
