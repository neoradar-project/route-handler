#pragma once
#include "Waypoint.h"
#include <optional>
#include <string>
#include <vector>
#include "ParsingError.h"
namespace RouteParser
{

  struct ParsedRoute
  {
    std::string rawRoute = "";
    std::vector<Waypoint> waypoints = {};
    std::vector<ParsingError> errors = {};
    int totalTokens = 0;
    std::optional<std::string> departureRunway = std::nullopt;
    std::optional<std::string> arrivalRunway = std::nullopt;
    std::optional<std::string> SID = std::nullopt;
    std::optional<std::string> STAR = std::nullopt;
  };
} // namespace RouteParser