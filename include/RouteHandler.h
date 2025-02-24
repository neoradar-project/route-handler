#pragma once
#include "Log.h"
#include "Navdata.h"
#include "Parser.h"
#include "types/Procedure.h"
#include <memory>
#include <string>
#include <thread>
#include <types/Waypoint.h>
#include <unordered_map>
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
    std::shared_ptr<NavdataObject> GetNavdata();

    void Bootstrap(ILogger logFunc, std::string navdataDbFile,
                   std::unordered_multimap<std::string, Procedure> procedures,
                   std::string airwaysDbFile)
    {
        Log::SetLogger(logFunc);
        navdata->SetProcedures(procedures);

        navdata->LoadAirwayNetwork(airwaysDbFile);
        navdata->LoadWaypoints(navdataDbFile);
        navdata->LoadAirports(navdataDbFile);

        Log::info("RouteHandler is ready.");
        this->isReady = true;
    }

    bool IsReady() { return this->isReady; }

private:
    std::shared_ptr<RouteParser::ParserHandler> Parser = nullptr;
    std::shared_ptr<RouteParser::NavdataObject> navdata = nullptr;
    bool isReady = false;
};