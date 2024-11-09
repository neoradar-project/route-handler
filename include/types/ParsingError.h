#pragma once
#include <string>

namespace RouteParser
{

    enum ParsingErrorLevel
    {
        INFO,
        ERROR
    };

    enum ParsingErrorType
    {
        ROUTE_EMPTY,
        PROCEDURE_RUNWAY_MISMATCH,
        UNKNOWN_PROCEDURE,
        UNKNOWN_WAYPOINT,
    };

    struct ParsingError
    {
        ParsingErrorType type;
        std::string message;
        int tokenIndex;
        std::string token;
        ParsingErrorLevel level;
    };

} // namespace RouteParser