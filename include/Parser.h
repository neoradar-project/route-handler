#include "Regexes.h"
#include "types/ParsedRoute.h"
#include "types/RouteWaypoint.h"
#include "types/Waypoint.h"
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
   * @brief Parses the first and last part of the route.
   * @param parsedRoute The parsed route object.
   * @param index The index of the current token.
   * @param token The current token being processed.
   * @param origin The origin of the route.
   * @param destination The destination of the route.
   * @param strict Whether to parse strictly or not, meaning that it will reject
   * if the found procedure is not in dataset
   */
  bool ParseFirstAndLastPart(ParsedRoute &parsedRoute, int index,
                             std::string token, std::string anchorIcao,
                             bool strict = false);

  /**
   * @brief Performs a waypoint check on the parsed route.
   * @param parsedRoute The parsed route object.
   * @param index The index of the current token.
   * @param token The current token being processed.
   * @param previousWaypoint The previous waypoint, if any.
   */
  bool ParseWaypoints(ParsedRoute &parsedRoute, int index, std::string token,
                      std::optional<Waypoint> &previousWaypoint);

  // 57N020W 59S030E 60N040W for no minutes, or 5220N03305E for minutes
  bool ParseLatLon(ParsedRoute &parsedRoute, int index, std::string token);

  std::optional<RouteWaypoint::PlannedAltitudeAndSpeed>
  ParsePlannedAltitudeAndSpeed(int index, std::string rightToken);

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