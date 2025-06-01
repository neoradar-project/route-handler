#pragma once
#include "erkir/geo/sphericalpoint.h"
#include <string>
#include "types/Waypoint.h"
namespace RouteParser
{
    enum class AirportType
    {
        LARGE_AIRPORT,
        SMALL_AIRPORT,
        HELIPORT,
        SEAPLANE_BASE,
        CLOSED,
        UNKNOWN
    };

    // Header-only helper function
    inline AirportType StringToAirportType(const std::string &type)
    {
        if (type == "large_airport")
            return AirportType::LARGE_AIRPORT;
        if (type == "small_airport")
            return AirportType::SMALL_AIRPORT;
        if (type == "heliport")
            return AirportType::HELIPORT;
        if (type == "seaplane_base")
            return AirportType::SEAPLANE_BASE;
        if (type == "closed")
            return AirportType::CLOSED;
        return AirportType::UNKNOWN;
    }

    class Airport
    {
    public:
        Airport() = default;

        inline Airport(const std::string &ident,
                       const std::string &name,
                       AirportType type,
                       const erkir::spherical::Point &position,
                       int elevation,
                       const std::string &isoCountry,
                       const std::string &isoRegion)
            : ident_(ident), name_(name), type_(type), position_(position), elevation_(elevation), isoCountry_(isoCountry), isoRegion_(isoRegion)
        {
        }

        // Inline getters
        inline const std::string &getIdent() const { return ident_; }
        inline const std::string &getName() const { return name_; }
        inline AirportType getType() const { return type_; }
        inline const erkir::spherical::Point &getPosition() const { return position_; }
        inline int getElevation() const { return elevation_; }
        inline const std::string &getIsoCountry() const { return isoCountry_; }
        inline const std::string &getIsoRegion() const { return isoRegion_; }

        // toWaypoint method
        inline Waypoint toWaypoint() const
        {
            return Waypoint(WaypointType::AIRPORT, ident_, name_, position_, 0);
        }

    private:
        std::string ident_;
        std::string name_;
        AirportType type_;
        erkir::spherical::Point position_;
        int elevation_;
        std::string isoCountry_;
        std::string isoRegion_;
    };
}
