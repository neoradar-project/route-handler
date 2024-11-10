#include "ctre.hpp"

namespace RouteParser {
class Regexes {
public:
  static constexpr auto RoutePlannedAltitudeAndSpeed =
      ctll::fixed_string{"^([NMK])([0-9]{3,4})([FASM])([0-9]{3,4})$"};
};
}; // namespace RouteParser