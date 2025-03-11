#include "RouteHandler.h"

std::shared_ptr<RouteParser::ParserHandler> RouteHandler::GetParser() { return this->parser; }

std::shared_ptr<RouteParser::AirportConfigurator> RouteHandler::GetAirportConfigurator()
{
    return this->airportConfigurator;
}

std::shared_ptr<RouteParser::NavdataObject> RouteHandler::GetNavdata() { return this->navdata; }