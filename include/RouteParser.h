#include "Navdata.h"
#include "SidStarParser.h"
#include "Utils.h"
#include "types/ParsedRoute.h"
#include <string>

namespace RouteParser {
class Parser {
public:
  ParsedRoute ParseRawRoute(std::string route, std::string origin,
                            std::string destination) {
    auto parsedRoute = ParsedRoute();
    parsedRoute.rawRoute = route;

    route = Utils::removeChar(route, ':');
    route = Utils::trimAndReduceSpaces(route);

    if (route.empty()) {
      parsedRoute.errors.push_back({ROUTE_EMPTY, "Route is empty", 0, ""});
      return parsedRoute;
    }

    const auto routeParts = Utils::splitBySpaces(route);
    parsedRoute.totalTokens = routeParts.size();

    auto previousWaypoint = NavdataContainer->FindAirport(origin);

    for (auto i = 0; i < routeParts.size(); i++) {
      const auto token = routeParts[i];

      if (token.empty() || token == origin || token == destination) {
        continue;
      }

      if (i == 0 || i == routeParts.size() - 1) {
        // Check for Sid and departure runway
        auto res =
            SidStarParser::FindProcedure(token, origin, i == 0 ? SID : STAR);
        if (i == 0) {
          parsedRoute.departureRunway = res.runway;
          parsedRoute.SID = res.procedure;
        } else {
          parsedRoute.arrivalRunway = res.runway;
          parsedRoute.STAR = res.procedure;
        }
        if (res.errors.size() > 0) {
          parsedRoute.errors.insert(parsedRoute.errors.end(),
                                    res.errors.begin(), res.errors.end());
        }
        if (res.extractedProcedure.has_value()) {
          parsedRoute.waypoints.insert(
              parsedRoute.waypoints.end(),
              res.extractedProcedure->waypoints.begin(),
              res.extractedProcedure->waypoints.end());
        }
      }

      // Check for airways

      // Check for waypoints
      auto waypoint =
          NavdataContainer->FindClosestWaypointTo(token, previousWaypoint);
      if (waypoint.has_value()) {
        parsedRoute.waypoints.push_back(waypoint.value());
        previousWaypoint = waypoint;
      } else {
        parsedRoute.errors.push_back(
            {UNKNOWN_WAYPOINT, "Unknown waypoint", i, token});
      }
    }

    return parsedRoute;
  }
};
} // namespace RouteParser