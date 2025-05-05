#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace RouteParser {

enum ParsingErrorLevel { INFO, PARSE_ERROR };

enum ParsingErrorType {
    ROUTE_EMPTY,
    PROCEDURE_RUNWAY_MISMATCH,
	PROCEDURE_AIRPORT_MISMATCH,
    UNKNOWN_PROCEDURE,
    UNKNOWN_WAYPOINT,
	NO_PROCEDURE_FOUND,
    INVALID_RUNWAY,
    INVALID_DATA,
    UNKNOWN_AIRPORT,     
    UNKNOWN_NAVAID,       
    UNKNOWN_AIRWAY,        
    INVALID_TOKEN_FORMAT,  
    INVALID_AIRWAY_FORMAT,
    INVALID_AIRWAY_DIRECTION,
    AIRWAY_FIX_NOT_FOUND,
    INSUFFICIENT_FLIGHT_LEVEL, 
    MULTIPLE_AIRWAYS_FOUND 
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