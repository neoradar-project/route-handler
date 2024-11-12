#include "AirwayParser.h"
#include <absl/strings/str_split.h>
#include <charconv>
#include <string_view>
#include "Utils.h"

namespace RouteParser
{

    std::optional<AirwayNetwork> AirwayParser::ParseAirwayTxt(std::string_view content)
    {
        if (content.empty())
        {
            return AirwayNetwork{};
        }

        std::vector<std::string_view> fields;
        fields.reserve(15);

        AirwayNetwork network;
        bool valid_data = false;

        for (std::string_view line : absl::StrSplit(content, '\n', absl::SkipEmpty()))
        {
            if (line.empty() || line[0] == ';' || absl::StripAsciiWhitespace(line).empty())
            {
                continue;
            }

            fields.clear();
            Utils::SplitAirwayFields(line, fields);

            static constexpr size_t MIN_FIELDS = 7;
            if (fields.size() < MIN_FIELDS)
            {
                continue;
            }

            if (fields[0].empty() || fields[1].empty() || fields[2].empty() ||
                fields[3].empty() || fields[4].empty() || fields[5].empty())
            {
                continue;
            }

            auto mainPoint = ParsePoint(fields[1], fields[2]);
            if (!mainPoint)
            {
                continue;
            }

            AirwayFix mainFix{
                std::string(fields[0]),
                *mainPoint};

            std::string_view airwayName = fields[4];
            std::string_view airwayType = fields[5];

            bool has_first_fix = fields.size() > 6 && fields[6] != "N";
            if (has_first_fix && fields.size() > 10)
            {
                if (auto neighborInfo = ParseNeighbour(fields, 6))
                {
                    bool added = network.addAirwaySegment(
                        std::string(airwayName),
                        std::string(airwayType),
                        mainFix,
                        AirwayFix{
                            neighborInfo->name,
                            neighborInfo->coord},
                        neighborInfo->minimum_level.value_or(0),
                        neighborInfo->valid_direction);
                }
            }

            size_t next_start = has_first_fix ? 11 : 7;
            if (fields.size() > next_start + 4 && fields[next_start] != "N")
            {
                if (auto neighborInfo = ParseNeighbour(fields, next_start))
                {
                    bool added = network.addAirwaySegment(
                        std::string(airwayName),
                        std::string(airwayType),
                        mainFix,
                        AirwayFix{
                            neighborInfo->name,
                            neighborInfo->coord},
                        neighborInfo->minimum_level.value_or(0),
                        neighborInfo->valid_direction);
                }
            }

            valid_data = true;
        }

        if (!valid_data && !content.empty())
        {
            return std::nullopt;
        }

        network.finalizeAirways();
        return network;
    }

    std::optional<AirwayParser::NeighbourInfo> AirwayParser::ParseNeighbour(
        const std::vector<std::string_view> &fields,
        size_t start_index)
    {
        if (fields.size() < start_index + 5)
        {
            return std::nullopt;
        }

        auto point = ParsePoint(fields[start_index + 1], fields[start_index + 2]);
        if (!point)
        {
            return std::nullopt;
        }

        auto level = ParseLevel(fields[start_index + 3]);

        return NeighbourInfo{
            std::string(fields[start_index]),
            *point,
            fields[start_index + 4] == "Y",
            level};
    }

    std::optional<erkir::spherical::Point> AirwayParser::ParsePoint(
        std::string_view lat,
        std::string_view lng)
    {
        double latitude, longitude;

        try
        {
            latitude = std::stod(std::string(lat));
            longitude = std::stod(std::string(lng));
        }
        catch (const std::invalid_argument &)
        {
            return std::nullopt;
        }
        catch (const std::out_of_range &)
        {
            return std::nullopt;
        }

        return erkir::spherical::Point(latitude, longitude);
    }

    std::optional<uint32_t> AirwayParser::ParseLevel(std::string_view level)
    {
        if (level == "NESTB")
        {
            return std::nullopt;
        }

        uint32_t result;
        const char *begin = level.data();
        const char *end = begin + level.size();
        auto [ptr, ec] = std::from_chars(begin, end, result);

        if (ec != std::errc{} || ptr != end)
        {
            return std::nullopt;
        }

        return result;
    }

} // namespace RouteParser