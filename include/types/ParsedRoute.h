#pragma once
#include "Waypoint.h"
#include <optional>
#include <string>
#include <vector>

namespace RouteParser {

enum ParsingErrorLevel { INFO, ERROR };

enum ParsingErrorType {
  ROUTE_EMPTY,
  PROCEDURE_RUNWAY_MISMATCH,
  UNKNOWN_PROCEDURE,
  UNKNOWN_WAYPOINT,
};

struct ParsingError {
  ParsingErrorType type;
  std::string message;
  int tokenIndex;
  std::string token;
  ParsingErrorLevel level;
};

struct ParsedRoute {
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