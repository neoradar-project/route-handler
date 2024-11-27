#pragma once
#include <string>
#include <erkir/geo/sphericalpoint.h>
#include "ParsingError.h"
#include "types/Waypoint.h" // Add this include

namespace RouteParser
{

    enum class AirwayLevel
    {
        BOTH, // 'B' - Both high and low
        HIGH, // 'H' - High level only
        LOW,  // 'L' - Low level only
        UNKNOWN
    };

    constexpr AirwayLevel stringToAirwayLevel(const char *level)
    {
        return (level[0] == 'B') ? AirwayLevel::BOTH : (level[0] == 'H') ? AirwayLevel::HIGH
                                                   : (level[0] == 'L')   ? AirwayLevel::LOW
                                                                         : AirwayLevel::UNKNOWN;
    }

    constexpr const char *airwayLevelToString(AirwayLevel level)
    {
        return (level == AirwayLevel::BOTH) ? "B" : (level == AirwayLevel::HIGH) ? "H"
                                                : (level == AirwayLevel::LOW)    ? "L"
                                                                                 : "U";
    }

    struct AirwaySegmentInfo
    {
        Waypoint from; // Changed from AirwayFix to Waypoint
        Waypoint to;   // Changed from AirwayFix to Waypoint
        uint32_t minimum_level;
        bool canTraverse;
    };

    struct RouteSegment
    {
        std::string from;
        std::string airwayName;
        std::string to;
    };

    struct RouteValidationResult
    {
        bool isValid;
        std::vector<ParsingError> errors;
        std::vector<AirwaySegmentInfo> segments;
        std::vector<Waypoint> path;
    };

} // namespace RouteParser