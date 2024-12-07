#pragma once
#include "Waypoint.h"
#include <string>

namespace RouteParser {

enum ProcedureType { PROCEDURE_SID, PROCEDURE_STAR };

struct Procedure {
    std::string name;
    std::string runway;
    std::string icao;
    ProcedureType type;
    std::vector<Waypoint> waypoints;
};
}; // namespace RouteParser