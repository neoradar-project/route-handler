#include "ctre.hpp"

namespace RouteParser {
class Regexes {
public:
  static constexpr auto RoutePlannedAltitudeAndSpeed =
      ctll::fixed_string{"^([NMK])([0-9]{3,4})([FASM])([0-9]{3,4})$"};

  static constexpr auto RouteLatLon =
      ctll::fixed_string{"^([0-9]{2})([0-9]{0,2})([NS])([0-9]{3})([0-9]{0,2})([EW])$"};
};
}; // namespace RouteParser