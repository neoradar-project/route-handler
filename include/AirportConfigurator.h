#pragma once
#include "Navdata.h"
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RouteParser {

struct AirportRunways {
    std::vector<std::string> depRunways;
    std::vector<std::string> arrRunways;
};

class AirportConfigurator {
public:
    AirportConfigurator() {};
    ~AirportConfigurator() {};

    inline void UpdateAirportRunways(
        const std::unordered_map<std::string, AirportRunways>& airportRunways)
    {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        runways_ = airportRunways;
    }

    std::vector<std::string> GetDepartureRunways(const std::string& icao) const
    {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        auto it = runways_.find(icao);
        if (it != runways_.end()) {
            return it->second.depRunways;
        }
        return {};
    }

    std::vector<std::string> GetArrivalRunways(const std::string& icao) const
    {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        auto it = runways_.find(icao);
        if (it != runways_.end()) {
            return it->second.arrRunways;
        }
        return {};
    }
    std::optional<std::pair<std::string, std::optional<Procedure>>> FindBestSID(
        const std::string& icao, const std::vector<std::string>& waypoints) const
    {
        if (waypoints.empty()) {
            return std::nullopt;
        }

        auto depRunways = GetDepartureRunways(icao);
        if (depRunways.empty()) {
            return std::nullopt;
        }

        // Create a set for O(1) runway lookups
        std::unordered_set<std::string> activeRunways(
            depRunways.begin(), depRunways.end());
        const auto& firstWaypoint = waypoints[0];
        const auto& procedures = NavdataObject::GetProcedures();
        auto airportProcedures = NavdataObject::GetProceduresByAirport(icao);

        // Find the first matching procedure
        for (size_t idx : airportProcedures) {
            const auto& procedure = procedures[idx];

            // Fast filters
            if (procedure.type != PROCEDURE_SID
                || !activeRunways.count(procedure.runway)) {
                continue;
            }

            // Check if the first waypoint is in this procedure
            for (const auto& procWpt : procedure.waypoints) {
                if (procWpt.getIdentifier() == firstWaypoint) {
                    // Direct return on first match for efficiency
                    return std::make_pair(procedure.runway, procedure);
                }
            }
        }

        // No match found
        return std::make_pair(depRunways[0], std::nullopt);
    }

    std::optional<std::pair<std::string, std::optional<Procedure>>> FindBestSTAR(
        const std::string& icao, const std::vector<std::string>& waypoints) const
    {
        if (waypoints.empty()) {
            return std::nullopt;
        }

        auto arrRunways = GetArrivalRunways(icao);
        if (arrRunways.empty()) {
            return std::nullopt;
        }

        // Create a set for O(1) runway lookups
        std::unordered_set<std::string> activeRunways(
            arrRunways.begin(), arrRunways.end());
        const auto& lastWaypoint = waypoints.back();
        const auto& procedures = NavdataObject::GetProcedures();
        auto airportProcedures = NavdataObject::GetProceduresByAirport(icao);

        // Find the first matching procedure
        for (size_t idx : airportProcedures) {
            const auto& procedure = procedures[idx];

            // Fast filters
            if (procedure.type != PROCEDURE_STAR
                || !activeRunways.count(procedure.runway)) {
                continue;
            }

            // Check if the last waypoint is in this procedure
            for (const auto& procWpt : procedure.waypoints) {
                if (procWpt.getIdentifier() == lastWaypoint) {
                    // Direct return on first match for efficiency
                    return std::make_pair(procedure.runway, procedure);
                }
            }
        }

        // No match found
        return std::make_pair(arrRunways[0], std::nullopt);
    }

private:
    std::unordered_map<std::string, AirportRunways> runways_;
    mutable std::mutex airportRunwaysMutex_;
};

} // namespace RouteParser