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
            SmallProceduresList{
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
        // Add NSE Waypoints
        static inline const std::vector<RouteParser::Waypoint> NseWaypointsList = {
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW27L", {51.46495277777779, -0.4340777777777778}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW27R", {51.47767500000001, -0.4332611111111111}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D130B", {51.46541666666668, -0.4262833333333333}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE34A", {51.42800555555555, -0.28491388888888886}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE29C", {51.410186111111095, -0.15488333333333332}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283T", {51.37774999999999, 0.07891388888888891}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283P", {51.36318333333332, 0.18271944444444446}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283E", {51.32265277777779, 0.4678388888888889}),
            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "DET", {51.30400277777778, 0.5972750000000001}, 117300),

            // Additional waypoints for ALESO1H STAR
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "LLE01", {51.187211111111104, 0.25602222222222226}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "BIG29", {51.36202500000002, -0.7344444444444445}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "GWC35", {51.437511111111135, -0.7892666666666667}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "CF09L", {51.476222222222226, -0.7514805555555557}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "FI09L", {51.47660000000001, -0.6848416666666668}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW09L", {51.477499999999985, -0.48501388888888897}),

            // Adding some common waypoints needed for the STAR
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ALESO", {51.15, 0.35}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ROTNO", {51.17, 0.32}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ETVAX", {51.18, 0.28}),
            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TIGER", {51.19, 0.26}),
            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "BIG", {51.33, -0.03}, 115300)
        };

        // Add the new STAR procedure to the procedures list
        static inline const std::unordered_multimap<std::string, RouteParser::Procedure>
            ExtendedProceduresList{
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
                { "ALESO1H",
                    RouteParser::Procedure { "ALESO1H", "09L", "EGLL",
                        RouteParser::ProcedureType::PROCEDURE_STAR,
                        {
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ALESO", {51.15, 0.35}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ROTNO", {51.17, 0.32}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ETVAX", {51.18, 0.28}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TIGER", {51.19, 0.26}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "LLE01", {51.187211111111104, 0.25602222222222226}),
                            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "BIG", {51.33, -0.03}, 115300),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "BIG29", {51.36202500000002, -0.7344444444444445}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "GWC35", {51.437511111111135, -0.7892666666666667}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "CF09L", {51.476222222222226, -0.7514805555555557}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "FI09L", {51.47660000000001, -0.6848416666666668}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW09L", {51.477499999999985, -0.48501388888888897})
                        }
                    }
                },
            { "ALESO2H",
                    RouteParser::Procedure { "ALESO2H", "09R", "EGLL",
                        RouteParser::ProcedureType::PROCEDURE_STAR,
                        {
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ALESO", {51.15, 0.35}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ROTNO", {51.17, 0.32}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ETVAX", {51.18, 0.28}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TIGER", {51.19, 0.26}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "LLE01", {51.187211111111104, 0.25602222222222226}),
                            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "BIG", {51.33, -0.03}, 115300),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "BIG29", {51.36202500000002, -0.7344444444444445}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "GWC35", {51.437511111111135, -0.7892666666666667}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "CF09L", {51.476222222222226, -0.7514805555555557}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "FI09L", {51.47660000000001, -0.6848416666666668}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW09L", {51.477499999999985, -0.48501388888888897})
                        }
                    }
                },
                { "DET1J",
                    RouteParser::Procedure { "DET1J", "09R", "EGLL",
                        RouteParser::ProcedureType::PROCEDURE_SID,
                        {
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW27L", {51.46495277777779, -0.4340777777777778}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D130B", {51.46541666666668, -0.4262833333333333}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE34A", {51.42800555555555, -0.28491388888888886}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE29C", {51.410186111111095, -0.15488333333333332}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283T", {51.37774999999999, 0.07891388888888891}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283P", {51.36318333333332, 0.18271944444444446}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283E", {51.32265277777779, 0.4678388888888889}),
                            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "DET", {51.30400277777778, 0.5972750000000001}, 117300)
                        }
                    }
                },
            { "DET2J",
                    RouteParser::Procedure { "DET2J", "09L", "EGLL",
                        RouteParser::ProcedureType::PROCEDURE_SID,
                        {
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "RW27R", {51.46495277777779, -0.4340777777777778}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D130B", {51.46541666666668, -0.4262833333333333}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE34A", {51.42800555555555, -0.28491388888888886}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DE29C", {51.410186111111095, -0.15488333333333332}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283T", {51.37774999999999, 0.07891388888888891}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283P", {51.36318333333332, 0.18271944444444446}),
                            RouteParser::Waypoint(RouteParser::WaypointType::FIX, "D283E", {51.32265277777779, 0.4678388888888889}),
                            RouteParser::Waypoint(RouteParser::WaypointType::VOR, "DET", {51.30400277777778, 0.5972750000000001}, 117300)
                        }
                    }
                }
        };
    }
} // namespace RouteHandlerTests