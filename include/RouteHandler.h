#pragma once
#include "Log.h"
#include "Navdata.h"
#include "Parser.h"
#include "types/Procedure.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <types/Waypoint.h>
#include <thread>
using namespace RouteParser;

class RouteHandler
{
public:
  RouteHandler()
  {
    this->navdata = std::make_shared<RouteParser::NavdataObject>();
    this->Parser = std::make_shared<RouteParser::ParserHandler>(this->navdata);
  }
  ~RouteHandler()
  {
    this->Parser.reset();
    this->navdata.reset();
  }

  std::shared_ptr<ParserHandler> GetParser();
  void Bootstrap(ILogger logFunc,
                 std::string waypointsFile,
                 std::unordered_multimap<std::string, Procedure> procedures,
                 std::string airwaysFile, std::string isecFile = "")
  {
    Log::SetLogger(logFunc);
    navdata->SetProcedures(procedures);

    navdata->LoadAirwayNetwork(airwaysFile);
    navdata->LoadWaypoints(waypointsFile);

    std::thread isecThread;
    if (!isecFile.empty())
    {
      isecThread = std::thread([this, &isecFile]()
                               { navdata->LoadIntersectionWaypoints(isecFile); });
    }

    if (!isecFile.empty() && isecThread.joinable())
    {
      isecThread.join();
    }

    Log::info("RouteHandler is ready.");
    this->isReady = true;
  }

private:
  std::shared_ptr<RouteParser::ParserHandler> Parser = nullptr;
  std::shared_ptr<RouteParser::NavdataObject> navdata = nullptr;
  bool isReady = false;
};