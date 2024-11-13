#pragma once
#include "Log.h"
#include "Navdata.h"
#include "Parser.h"
#include "types/Procedure.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <types/Waypoint.h>
#include <future>
using namespace RouteParser;

class RouteHandler
{
public:
  RouteHandler() { this->Parser = std::make_shared<RouteParser::ParserHandler>(); }
  ~RouteHandler() { this->Parser.reset(); }

  std::shared_ptr<ParserHandler> GetParser();
  void Bootstrap(ILogger logFunc,
                 std::unordered_map<std::string, Waypoint> waypoints,
                 std::unordered_multimap<std::string, Procedure> procedures,
                 std::string airwaysFile, std::string isecFile = "")
  {
    Log::SetLogger(logFunc);
    NavdataObject::SetWaypoints(waypoints, procedures);

    // Create asynchronous tasks for loading airways and intersection waypoints
    std::future<void> airwaysTask = std::async(std::launch::async, [&]()
                                               { NavdataObject::LoadAirwayNetwork(airwaysFile); });

    std::future<void> isecTask;
    if (!isecFile.empty())
    {
      isecTask = std::async(std::launch::async, [&]()
                            { NavdataObject::LoadIntersectionWaypoints(isecFile); });
    }

    // Wait for both tasks to complete
    airwaysTask.wait();
    if (isecTask.valid())
    {
      isecTask.wait();
    }

    Log::info("RouteHandler is ready.");
    this->isReady = true;
  }

private:
  std::shared_ptr<ParserHandler> Parser = nullptr;
  bool isReady = false;
};