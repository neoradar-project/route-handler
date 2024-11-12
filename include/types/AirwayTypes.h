#pragma once
#include <string>
#include <erkir/geo/sphericalpoint.h>
#include "ParsingError.h"
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

    struct AirwayFix
    {
        std::string name;
        erkir::spherical::Point coord;

        bool operator==(const AirwayFix &other) const
        {
            return name == other.name;
        }
    };

    struct AirwaySegmentInfo
    {
        AirwayFix from;
        AirwayFix to;
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
    };

} // namespace RouteParser