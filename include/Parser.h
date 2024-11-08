#include "types/ParsedRoute.h"
#include <string>

namespace RouteParser {

/**
 * @class Parser
 * @brief A class to parse and handle routes.
 */
class ParserHandler {
private:
  /**
   * @brief Cleans up the raw route string.
   * @param route The raw route string to be cleaned.
   * @return A cleaned-up route string.
   */
  std::string CleanupRawRoute(std::string route);

  /**
   * @brief Handles the first and last parts of the parsed route.
   * @param parsedRoute The parsed route object.
   * @param index The index of the current token.
   * @param token The current token being processed.
   * @param origin The origin of the route.
   * @param destination The destination of the route.
   */
  void HandleFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                              std::string token, std::string origin,
                              std::string destination);

  /**
   * @brief Performs a waypoint check on the parsed route.
   * @param parsedRoute The parsed route object.
   * @param index The index of the current token.
   * @param token The current token being processed.
   * @param previousWaypoint The previous waypoint, if any.
   */
  void DoWaypointsCheck(ParsedRoute &parsedRoute, int index, std::string token,
                        std::optional<Waypoint> &previousWaypoint);

public:
  /**
   * @brief Parses a raw route string into a ParsedRoute object.
   * @param route The raw route string to be parsed.
   * @param origin The origin of the route.
   * @param destination The destination of the route.
   * @return A ParsedRoute object representing the parsed route.
   */
  ParsedRoute ParseRawRoute(std::string route, std::string origin,
                            std::string destination);
};

} // namespace RouteParser