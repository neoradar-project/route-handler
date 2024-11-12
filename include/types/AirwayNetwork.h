#pragma once
#include "Airway.h"
#include "AirwayTypes.h"
#include "types/Waypoint.h" // Add this include
#include <erkir/geo/sphericalpoint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <map>
#include <erkir/geo/sphericalpoint.h>
#include "Airway.h"
#include "AirwayTypes.h"

namespace RouteParser
{
    class AirwayNetwork
    {
    private:
        std::unordered_multimap<std::string, std::shared_ptr<Airway>> airways;
        std::shared_ptr<const Airway>
        findNearestAirway(const std::string &name,
                          const erkir::spherical::Point &point) const;

    public:
        RouteValidationResult validateAirwayTraversal(
            const std::string &startFix,
            const std::string &airwayName,
            const std::string &endFix,
            uint32_t flightLevel,
            const erkir::spherical::Point &nearPoint) const;

        RouteValidationResult validateRoute(const std::string &routeString) const;
        RouteValidationResult validateSegments(const std::vector<RouteSegment> &segments,
                                               const erkir::spherical::Point &startPoint) const;
        std::vector<std::shared_ptr<const Airway>> getAirwaysByNameAndFix(const std::string &name,
                                                                          const std::string &fixName) const;
        bool addAirwaySegment(const std::string &airwayName,
                              const std::string &levelChar,
                              const Waypoint &from,
                              const Waypoint &to,
                              uint32_t minLevel,
                              bool canTraverse);
        std::vector<AirwaySegmentInfo> getSegmentsBetween(const std::string &airwayName,
                                                          const std::string &from,
                                                          const std::string &to,
                                                          const erkir::spherical::Point &nearPoint) const;
        std::vector<Waypoint> getFixesBetween(const std::string &airwayName,
                                              const std::string &from,
                                              const std::string &to,
                                              const erkir::spherical::Point &nearPoint) const;
        std::vector<std::string> getAllAirways() const;
        std::shared_ptr<const Airway> getAirway(const std::string &name) const;
        std::shared_ptr<const Airway> getAirwayByLatLon(const std::string &name,
                                                        const erkir::spherical::Point &latLon) const;
        void finalizeAirways();
    };
} // namespace RouteParser