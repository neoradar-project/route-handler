#pragma once
#include "Converters.h"
#include "Units.h"
#include "Waypoint.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace RouteParser {

class RouteWaypoint : public Waypoint {
public:
    struct PlannedAltitudeAndSpeed {
        std::optional<int> plannedAltitude = std::nullopt;
        std::optional<int> plannedSpeed = std::nullopt;
        Units::Distance altitudeUnit = Units::Distance::FEET;
        Units::Speed speedUnit = Units::Speed::KNOTS;

        friend void to_json(nlohmann::json& j, const PlannedAltitudeAndSpeed& p)
        {
            j = nlohmann::json { { "plannedAltitude",
                                     p.plannedAltitude
                                         ? nlohmann::json(*p.plannedAltitude)
                                         : nlohmann::json(nullptr) },
                { "plannedSpeed",
                    p.plannedSpeed ? nlohmann::json(*p.plannedSpeed)
                                   : nlohmann::json(nullptr) },
                { "altitudeUnit", p.altitudeUnit }, { "speedUnit", p.speedUnit } };
        }

        friend void from_json(const nlohmann::json& j, PlannedAltitudeAndSpeed& p)
        {
            if (j.at("plannedAltitude").is_null()) {
                p.plannedAltitude = std::nullopt;
                if (j.at("plannedSpeed").is_null()) {
                    p.plannedSpeed = std::nullopt;
                } else {
                    p.plannedSpeed = j.at("plannedSpeed").get<int>();
                }
                p.plannedAltitude = j.at("plannedAltitude").get<int>();
            }

            j.at("altitudeUnit").get_to(p.altitudeUnit);
            j.at("speedUnit").get_to(p.speedUnit);
        }
    };

    RouteWaypoint() = default;

    RouteWaypoint(WaypointType type, std::string identifier,
        erkir::spherical::Point position, int frequencyHz = 0,
        FlightRule flightRule = IFR,
        std::optional<PlannedAltitudeAndSpeed> plannedPosition = std::nullopt)
        : Waypoint(type, identifier, position, frequencyHz)
    {
        m_plannedPosition = plannedPosition; // Changed to m_ prefix
        m_flightRule = flightRule; // Changed to m_ prefix
    }

    std::optional<PlannedAltitudeAndSpeed> GetPlannedPosition() const
    {
        return m_plannedPosition;
    }

    FlightRule GetFlightRule() const { return m_flightRule; }

    std::optional<PlannedAltitudeAndSpeed> m_plannedPosition = std::nullopt;
    FlightRule m_flightRule = IFR;
};

inline void to_json(nlohmann::json& j, const RouteWaypoint& wp)
{
    to_json(j, static_cast<const Waypoint&>(wp));
    j["plannedPosition"] = wp.m_plannedPosition ? nlohmann::json(*wp.m_plannedPosition)
                                                : nlohmann::json(nullptr);
    j["flightRule"] = wp.m_flightRule;
}

inline void from_json(const nlohmann::json& j, RouteWaypoint& wp)
{
    from_json(j, static_cast<Waypoint&>(wp));
    if (j.at("plannedPosition").is_null()) {
        wp.m_plannedPosition = std::nullopt;
    } else {
        wp.m_plannedPosition
            = j.at("plannedPosition").get<RouteWaypoint::PlannedAltitudeAndSpeed>();
    }
    j.at("flightRule").get_to(wp.m_flightRule);
}

inline void to_json(nlohmann::json& j, const std::vector<RouteWaypoint>& waypoints)
{
    j = nlohmann::json::array();
    for (const auto& waypoint : waypoints) {
        j.push_back(nlohmann::json(waypoint));
    }
}

inline void from_json(const nlohmann::json& j, std::vector<RouteWaypoint>& waypoints)
{
    waypoints.clear();
    for (const auto& element : j) {
        RouteWaypoint wp;
        from_json(element, wp);
        waypoints.push_back(wp);
    }
}

} // namespace RouteParser