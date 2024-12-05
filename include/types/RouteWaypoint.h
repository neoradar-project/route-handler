#pragma once
#include "Units.h"
#include "Waypoint.h"
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
namespace RouteParser
{

  class RouteWaypoint : public Waypoint
  {
  public:
    struct PlannedAltitudeAndSpeed
    {
      std::optional<int> plannedAltitude = std::nullopt;
      std::optional<int> plannedSpeed = std::nullopt;
      Units::Distance altitudeUnit = Units::Distance::FEET;
      Units::Speed speedUnit = Units::Speed::KNOTS;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlannedAltitudeAndSpeed,
                                     plannedAltitude, plannedSpeed, altitudeUnit, speedUnit)
    };

    RouteWaypoint(
        WaypointType type, std::string identifier,
        erkir::spherical::Point position, int frequencyHz = 0,
        FlightRule flightRule = IFR,
        std::optional<PlannedAltitudeAndSpeed> plannedPosition = std::nullopt)
        : Waypoint(type, identifier, position, frequencyHz)
    {
      m_plannedPosition = plannedPosition; // Changed to m_ prefix
      m_flightRule = flightRule;           // Changed to m_ prefix
    }

    std::optional<PlannedAltitudeAndSpeed> GetPlannedPosition() const
    {
      return m_plannedPosition;
    }

    FlightRule GetFlightRule() const { return m_flightRule; }

    friend void to_json(nlohmann::json &j, const RouteWaypoint &wp)
    {
      to_json(j, static_cast<const Waypoint &>(wp));
      j["plannedPosition"] = wp.m_plannedPosition;
      j["flightRule"] = wp.m_flightRule;
    }

    friend void from_json(const nlohmann::json &j, RouteWaypoint &wp)
    {
      from_json(j, static_cast<Waypoint &>(wp));
      j.at("plannedPosition").get_to(wp.m_plannedPosition);
      j.at("flightRule").get_to(wp.m_flightRule);
    }

  private:
    std::optional<PlannedAltitudeAndSpeed> m_plannedPosition = std::nullopt;
    FlightRule m_flightRule = IFR;
  };

} // namespace RouteParser