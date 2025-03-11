#pragma once
#include "ParsingError.h"
#include "RouteWaypoint.h"
#include "Procedure.h"
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
        // Basic route information
        std::string rawRoute = "";
        std::vector<RouteWaypoint> waypoints = {};
        std::vector<ParsingError> errors = {};
        std::vector<ParsedRouteSegment> segments = {};
        int totalTokens = 0;

        // Runway information
        std::optional<std::string> departureRunway = std::nullopt;
        std::optional<std::string> arrivalRunway = std::nullopt;

        // Actual procedures
        std::optional<Procedure> SID = std::nullopt;
        std::optional<Procedure> STAR = std::nullopt;

        // Suggested procedures and runways
        std::optional<std::string> suggestedDepartureRunway = std::nullopt;
        std::optional<std::string> suggestedArrivalRunway = std::nullopt;
        std::optional<Procedure> suggestedSID = std::nullopt;
        std::optional<Procedure> suggestedSTAR = std::nullopt;

        // Complete route with all segments (SID + route + STAR)
        std::vector<ParsedRouteSegment> explicitSegments = {};
        std::vector<RouteWaypoint> explicitWaypoints = {};

        // Connection points
        std::optional<std::string> sidConnectionWaypoint = std::nullopt;
        std::optional<std::string> starConnectionWaypoint = std::nullopt;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            ParsedRoute,
            rawRoute,
            waypoints,
            errors,
            segments,
            totalTokens,
            departureRunway,
            arrivalRunway,
            SID,
            STAR,
            suggestedDepartureRunway,
            suggestedArrivalRunway,
            suggestedSID,
            suggestedSTAR,
            explicitSegments,
            explicitWaypoints,
            sidConnectionWaypoint,
            starConnectionWaypoint
        )
    };
} // namespace RouteParser