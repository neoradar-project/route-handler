#include "types/Procedure.h"
#include "types/Waypoint.h"
#include <string>
#include <unordered_map>

namespace RouteHandlerTests {
namespace Data {
static inline const std::unordered_multimap<std::string, RouteParser::Waypoint>
    SmallWaypointsList = {
        { "PAINT",
            RouteParser::Waypoint(
                RouteParser::WaypointType::FIX, "PAINT", { 51.250000, -0.420000 }) },
        { "BLUE",
            RouteParser::Waypoint(
                RouteParser::WaypointType::FIX, "BLUE", { 51.320000, -0.280000 }) },
        { "TESIG",
            RouteParser::Waypoint(
                RouteParser::WaypointType::FIX, "TESIG", { 51.380000, -0.150000 }) },
        { "DOTMI",
            RouteParser::Waypoint(
                RouteParser::WaypointType::FIX, "DOTMI", { 51.450000, -0.050000 }) },
        { "ABBEY",
            RouteParser::Waypoint(
                RouteParser::WaypointType::FIX, "ABBEY", { 51.510000, 0.080000 }) },
    };

static inline const std::unordered_multimap<std::string, RouteParser::Procedure>
    SmallProceduresList {
        { "ABBEY3A",
            RouteParser::Procedure { "ABBEY3A", "07R", "VHHH",
                RouteParser::ProcedureType::PROCEDURE_STAR,
                { RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ABBEY",
                    { 51.510000, 0.080000 }) } } },
        { "TES61X",
            RouteParser::Procedure { "TES61X", "06", "ZSNJ",
                RouteParser::ProcedureType::PROCEDURE_SID,
                { RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TESIG",
                    { 51.380000, -0.150000 }) } } },
    };
}

} // namespace RouteHandlerTests