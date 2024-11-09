#include "Parser.h"
#include "Navdata.h"
#include "SidStarParser.h"
#include "Utils.h"
#include <iostream>
using namespace RouteParser;

std::string ParserHandler::CleanupRawRoute(std::string route)
{
  route = Utils::removeChar(route, ':');
  route = Utils::trimAndReduceSpaces(route);
  return route;
}

void ParserHandler::HandleFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                                           std::string token,
                                           std::string origin,
                                           std::string destination)
{
  // Check for Sid and departure runway
  auto res =
      SidStarParser::FindProcedure(token, origin, index == 0 ? SID : STAR);
  if (index == 0)
  {
    parsedRoute.departureRunway = res.runway;
    parsedRoute.SID = res.procedure;
  }
  else
  {
    parsedRoute.arrivalRunway = res.runway;
    parsedRoute.STAR = res.procedure;
  }
  if (res.errors.size() > 0)
  {
    parsedRoute.errors.insert(parsedRoute.errors.end(), res.errors.begin(),
                              res.errors.end());
  }
  if (res.extractedProcedure.has_value())
  {
    parsedRoute.waypoints.insert(parsedRoute.waypoints.end(),
                                 res.extractedProcedure->waypoints.begin(),
                                 res.extractedProcedure->waypoints.end());
  }
}

void ParserHandler::DoWaypointsCheck(
    ParsedRoute &parsedRoute, int index, std::string token,
    std::optional<Waypoint> &previousWaypoint)
{
  auto waypoint =
      NavdataContainer->FindClosestWaypointTo(token, previousWaypoint);
  if (waypoint.has_value())
  {
    parsedRoute.waypoints.push_back(waypoint.value());
    previousWaypoint = waypoint;
  }
  else
  {
    parsedRoute.errors.push_back(
        {UNKNOWN_WAYPOINT, "Unknown waypoint", index, token});
  }
}

ParsedRoute ParserHandler::ParseRawRoute(std::string route, std::string origin,
                                         std::string destination)
{
  auto parsedRoute = ParsedRoute();
  parsedRoute.rawRoute = route;

  route = CleanupRawRoute(route);

  if (route.empty())
  {
    parsedRoute.errors.push_back({ROUTE_EMPTY, "Route is empty", 0, ""});
    return parsedRoute;
  }

  const auto routeParts = Utils::splitBySpaces(route);
  parsedRoute.totalTokens = routeParts.size();

  auto previousWaypoint = NavdataContainer->FindWaypointByType(origin, AIRPORT);

  for (auto i = 0; i < routeParts.size(); i++)
  {
    const auto token = routeParts[i];

    if (token.empty() || token == origin || token == destination ||
        token == "DCT")
    {
      continue;
    }

    if (i == 0 || i == routeParts.size() - 1)
    {
      HandleFirstAndLastPart(parsedRoute, i, token, origin, destination);
    }

    // Check for airways

    // Check for waypoints
    this->DoWaypointsCheck(parsedRoute, i, token, previousWaypoint);
  }

  return parsedRoute;
}