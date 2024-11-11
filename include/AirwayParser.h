#pragma once
#include <optional>
#include <vector>
#include <string_view>
#include "types/Airway.h"

namespace RouteParser
{

    class AirwayParser
    {
    public:
        static std::optional<AirwayNetwork> ParseAirwayTxt(std::string_view content);

    private:
        struct NeighbourInfo
        {
            std::string name;
            erkir::spherical::Point coord;
            bool valid_direction;
            std::optional<uint32_t> minimum_level;
        };

        static std::optional<NeighbourInfo> ParseNeighbour(
            const std::vector<std::string_view> &fields,
            size_t start_index);

        static std::optional<erkir::spherical::Point> ParsePoint(
            std::string_view lat,
            std::string_view lng);

        static std::optional<uint32_t> ParseLevel(std::string_view level);
    };

} // namespace RouteParser