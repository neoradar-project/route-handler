#include "RouteHandler.h"

std::shared_ptr<RouteParser::ParserHandler> RouteHandler::GetParser() { return this->Parser; }

std::shared_ptr<RouteParser::NavdataObject> RouteHandler::GetNavdata() { return this->navdata; }