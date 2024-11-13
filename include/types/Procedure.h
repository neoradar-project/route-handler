#pragma once
#include "Waypoint.h"
#include <string>

namespace RouteParser
{

  enum ProcedureType
  {
    SID,
    STAR
  };

  struct Procedure
  {
    std::string name;
    std::string runway;
    std::string icao;
    ProcedureType type;
    std::vector<Waypoint> waypoints;
  };
}; // namespace RouteParser