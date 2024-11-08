#include "RouteHandler.h"

std::shared_ptr<RouteParser::Parser> RouteHandler::GetParser() { return this->Parser; }