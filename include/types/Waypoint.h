#pragma once
#include "Converters.h"
#include "erkir/geo/sphericalpoint.h"
#include <string>

namespace RouteParser {

enum WaypointType {
    FIX,
    VOR,
    DME,
    VORDME,
    NDBDME,
    VORTAC,
    TACAN,
    NDB,
    AIRPORT,
    LATLON,
    UNKNOWN
};

class Waypoint {
public:
    Waypoint()
        : type(UNKNOWN)
        , frequencyHz(0)
    {
        // Initialize the default values for the member variables
    }

    Waypoint(WaypointType type, std::string identifier, std::string name, erkir::spherical::Point position,
        int frequencyHz = 0)
    {
        this->type = type;
        this->name = name;
        this->identifier = identifier;
        this->position = position;
        this->frequencyHz = frequencyHz;
    }
    WaypointType getType() const { return type; };
        std::string getName() const { return name; };
    std::string getIdentifier() const { return identifier; };
    erkir::spherical::Point getPosition() const { return position; };
    int getFrequencyHz() const { return frequencyHz; };

    double distanceToInMeters(Waypoint other) const
    {
        return this->position.distanceTo(other.getPosition());
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Waypoint, name, type, identifier, position, frequencyHz);

private:
    WaypointType type;
    std::string identifier;
    std::string name;
    int frequencyHz;
    erkir::spherical::Point position;
};

} // namespace RouteParser