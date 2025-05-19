#pragma once
#include "AirportConfigurator.h"
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

class RouteHandler {
public:
    RouteHandler()
    {
        this->airportConfigurator = std::make_shared<RouteParser::AirportConfigurator>();
        this->navdata = std::make_shared<RouteParser::NavdataObject>();
        this->parser = std::make_shared<RouteParser::ParserHandler>(
            this->navdata, this->airportConfigurator);
    }
    ~RouteHandler()
    {
        this->parser.reset();
        this->navdata.reset();
    }

    std::shared_ptr<RouteParser::ParserHandler> GetParser();
    std::shared_ptr<RouteParser::NavdataObject> GetNavdata();
    std::shared_ptr<RouteParser::AirportConfigurator> GetAirportConfigurator();

    void Bootstrap(ILogger logFunc, std::string navdataDbFile,
        std::vector<Procedure> procedures, std::string airwaysDbFile)
    {
        Log::SetLogger(logFunc);
        navdata->SetProcedures(procedures);

        navdata->LoadAirwayNetwork(airwaysDbFile);
        navdata->LoadWaypoints(navdataDbFile);
        navdata->LoadAirports(navdataDbFile);
        navdata->LoadRunways(navdataDbFile);

        Log::info("RouteHandler is ready.");
        this->isReady = true;
    }

    bool IsReady() { return this->isReady; }

private:
    std::shared_ptr<RouteParser::ParserHandler> parser = nullptr;
    std::shared_ptr<RouteParser::NavdataObject> navdata = nullptr;
    std::shared_ptr<RouteParser::AirportConfigurator> airportConfigurator = nullptr;
    bool isReady = false;
};