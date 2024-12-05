#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace RouteParser {

enum ParsingErrorLevel { INFO, ERROR };

enum ParsingErrorType {
    ROUTE_EMPTY,
    PROCEDURE_RUNWAY_MISMATCH,
    UNKNOWN_PROCEDURE,
    UNKNOWN_WAYPOINT,
    INVALID_RUNWAY,
    INVALID_DATA,

    INVALID_AIRWAY_FORMAT,
    UNKNOWN_AIRWAY,
    INVALID_AIRWAY_DIRECTION,
    AIRWAY_FIX_NOT_FOUND,
    INSUFFICIENT_FLIGHT_LEVEL, // Add this new error type
    MULTIPLE_AIRWAYS_FOUND // Add this new error type
};

struct ParsingError {
    ParsingErrorType type;
    std::string message;
    int tokenIndex;
    std::string token;
    ParsingErrorLevel level;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ParsingError, type, message, tokenIndex, token, level);
};

} // namespace RouteParser