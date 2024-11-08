#include "RouteHandler.h"

std::shared_ptr<RouteParser::ParserHandler> RouteHandler::GetParser() { return this->Parser; }