#pragma once
#include "ParsingError.h"
#include "RouteWaypoint.h"
#include <optional>
#include <string>
#include <vector>

namespace RouteParser {
struct ParsedRouteSegment {
    RouteWaypoint from;
    RouteWaypoint to;
    std::string airway; // "DCT" for direct connections
    int minimumLevel = -1;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ParsedRouteSegment, from, to, airway, minimumLevel);
};
struct ParsedRoute {
    std::string rawRoute = "";
    std::vector<RouteWaypoint> waypoints = {};
    std::vector<ParsingError> errors = {};
    std::vector<ParsedRouteSegment> segments = {};
    int totalTokens = 0;
    std::optional<std::string> departureRunway = std::nullopt;
    std::optional<std::string> arrivalRunway = std::nullopt;
    std::optional<std::string> SID = std::nullopt;
    std::optional<std::string> STAR = std::nullopt;
};
} // namespace RouteParser