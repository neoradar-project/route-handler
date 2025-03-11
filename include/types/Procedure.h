#pragma once
#include "Waypoint.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace RouteParser {
    enum ProcedureType { PROCEDURE_SID, PROCEDURE_STAR };

    struct Procedure {
        std::string name;
        std::string runway;
        std::string icao;
        ProcedureType type;
        std::vector<Waypoint> waypoints;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Procedure, name, runway, icao, type, waypoints)
    };
}; // namespace RouteParser