#pragma once
#include "Log.h"
#include "Navdata.h"
#include "Parser.h"
#include "types/Procedure.h"
#include <map>
#include <memory>
#include <string>
#include <types/Waypoint.h>

using namespace RouteParser;

class RouteHandler {
public:
  RouteHandler() { this->Parser = std::make_shared<RouteParser::Parser>(); }
  ~RouteHandler() { this->Parser.reset(); }

  std::shared_ptr<Parser> GetParser();
  void Bootstrap(ILogger logFunc,
                 std::multimap<std::string, Waypoint> waypoints,
                 std::multimap<std::string, Procedure> procedures) {
    Log::SetLogger(logFunc);
    NavdataContainer->SetWaypoints(waypoints, procedures);

    Log::info("RouteHandler is ready.");
    this->isReady = true;
  };

private:
  std::shared_ptr<Parser> Parser = nullptr;
  bool isReady = false;
};