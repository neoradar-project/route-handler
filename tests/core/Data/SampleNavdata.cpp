#include "types/Waypoint.h"
#include "types/Procedure.h"
#include <unordered_map>
#include <string>

namespace RouteHandlerTests
{
    namespace Data
    {
        static inline const std::unordered_map<std::string, RouteParser::Waypoint>
            SmallWaypointsList = {
                {"PAINT", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "PAINT",
                                                {51.250000, -0.420000})},
                {"BLUE",
                 RouteParser::Waypoint(RouteParser::WaypointType::FIX, "BLUE", {51.320000, -0.280000})},
                {"TESIG", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TESIG",
                                                {51.380000, -0.150000})},
                {"DOTMI", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "DOTMI",
                                                {51.450000, -0.050000})},
                {"ABBEY", RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ABBEY",
                                                {51.510000, 0.080000})},
        };

        static inline const std::unordered_multimap<std::string, RouteParser::Procedure> SmallProceduresList{
            {"ABBEY3A", RouteParser::Procedure{"ABBEY3A", "07R", "VHHH", RouteParser::ProcedureType::STAR, {RouteParser::Waypoint(RouteParser::WaypointType::FIX, "ABBEY", {51.510000, 0.080000})}}},
            {"TES61X", RouteParser::Procedure{"TES61X", "06", "ZSNJ", RouteParser::ProcedureType::SID, {RouteParser::Waypoint(RouteParser::WaypointType::FIX, "TESIG", {51.380000, -0.150000})}}},
        };

        static inline const char *SmallAirwayList =
            // A470 Airway - Connecting TESIG to DOTMI
            "TESIG\t51.380000\t-0.150000\t14\tA470\tB\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "N\n"

            "DOTMI\t51.450000\t-0.050000\t14\tA470\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tN\t"
            "N\n"

            // V512 Airway - Connecting TESIG to ABBEY (direct + via DOTMI)
            "TESIG\t51.380000\t-0.150000\t14\tV512\tB\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\t"
            "N\n"

            "ABBEY\t51.510000\t0.080000\t14\tV512\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "N\n"

            // Original V512 DOTMI-ABBEY connection
            "DOTMI\t51.450000\t-0.050000\t14\tV512\tB\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\t"
            "N\n"

            "ABBEY\t51.510000\t0.080000\t14\tV512\tB\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tN\t"
            "N\n"

            // B123 Airway - Bidirectional PAINT -> BLUE -> TESIG -> DOTMI
            "PAINT\t51.250000\t-0.420000\t14\tB123\tB\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "BLUE\t51.320000\t-0.280000\t14\tB123\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tB123\tB\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\n"

            "DOTMI\t51.450000\t-0.050000\t14\tB123\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tB123\tB\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\t"
            "PAINT\t51.250000\t-0.420000\t18000\tY\n"

            // L45 Airway - Unidirectional PAINT -> BLUE -> TESIG -> DOTMI -> ABBEY
            "PAINT\t51.250000\t-0.420000\t14\tL45\tH\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "BLUE\t51.320000\t-0.280000\t14\tL45\tH\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tL45\tH\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\n"

            // M20 Airway - Unidirectional ABBEY -> DOTMI -> TESIG -> BLUE -> PAINT
            "ABBEY\t51.510000\t0.080000\t14\tM20\tH\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "DOTMI\t51.450000\t-0.050000\t14\tM20\tH\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tM20\tH\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\t"
            "PAINT\t51.250000\t-0.420000\t18000\tY\n"

            // R10 Airway - Bidirectional between ABBEY <-> DOTMI <-> TESIG <-> BLUE
            "ABBEY\t51.510000\t0.080000\t14\tR10\tB\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "DOTMI\t51.450000\t-0.050000\t14\tR10\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tR10\tB\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\n"

            "DOTMI\t51.450000\t-0.050000\t14\tR10\tB\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "BLUE\t51.320000\t-0.280000\t14\tR10\tB\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\n"

            // P44 Airway - Unidirectional PAINT -> BLUE -> TESIG -> DOTMI -> ABBEY
            "PAINT\t51.250000\t-0.420000\t14\tP44\tH\t"
            "BLUE\t51.320000\t-0.280000\t18000\tY\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\n"

            "BLUE\t51.320000\t-0.280000\t14\tP44\tH\t"
            "TESIG\t51.380000\t-0.150000\t18000\tY\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\n"

            "TESIG\t51.380000\t-0.150000\t14\tP44\tH\t"
            "DOTMI\t51.450000\t-0.050000\t18000\tY\t"
            "ABBEY\t51.510000\t0.080000\t18000\tY\n";
    }
} // namespace RouteHandlerTests