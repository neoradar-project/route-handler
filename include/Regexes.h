#pragma once

#include "ctre.hpp"

namespace RouteParser
{
    class Regexes
    {
    public:
        static constexpr auto RoutePlannedAltitudeAndSpeed = ctll::fixed_string{
            "^((M)(\\d{3})|([NK])(\\d{4}))(([FA])(\\d{3})|([SM])(\\d{4}))$"};

        static constexpr auto RouteLatLon = ctll::fixed_string{
            "^([0-9]{2})([0-9]{0,2})([NS])([0-9]{3})([0-9]{0,2})([EW])$"};

        static constexpr auto RouteNDB = ctll::fixed_string{"^[A-Z]{1,3}$"};

        static constexpr auto RouteVOR = ctll::fixed_string{"^[A-Z]{3}$"};

        static constexpr auto RouteFIX = ctll::fixed_string{"^[A-Z]{5}$"};
    };
} // namespace RouteParser