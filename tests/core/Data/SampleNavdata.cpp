#include "types/Waypoint.h"
#include <map>
#include <string>

namespace RouteHandlerTests {
namespace Data {
static inline const std::multimap<std::string, RouteParser::Waypoint>
    SmallWaypointsList = {
        {"PAINT", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "PAINT",
                                        {0, 0})},
        {"BLUE",
         RouteParser::Waypoint(RouteParser::WaypointType::FIX, "BLUE", {0, 0})},
        {"TESIG", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TESIG",
                                        {0, 0})},
        {"DOTMI", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DOTMI",
                                        {0, 0})},
        {"ABBEY", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ABBEY",
                                        {0, 0})},
};
}
} // namespace RouteHandlerTests