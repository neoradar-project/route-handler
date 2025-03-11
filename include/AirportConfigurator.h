#pragma once

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

    std::vector<std::string> GetDepartureRunways(const std::string& icao) const {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        auto it = runways_.find(icao);
        if (it != runways_.end()) {
            return it->second.depRunways;
        }
        return {};
    }

    std::vector<std::string> GetArrivalRunways(const std::string& icao) const {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        auto it = runways_.find(icao);
        if (it != runways_.end()) {
            return it->second.arrRunways;
        }
        return {};
    }

    std::optional<std::pair<std::string, std::string>> FindBestSID(
        const std::string& icao,
        const std::vector<std::string>& waypoints) const {

        if (waypoints.empty()) {
            return std::nullopt;
        }

        // Get active departure runways
        auto depRunways = GetDepartureRunways(icao);
        if (depRunways.empty()) {
            return std::nullopt;
        }

        // Get the first waypoint in the route
        const auto& firstWaypoint = waypoints[0];

        // Get all procedures for this airport
        auto procedures = NavdataObject::GetProcedures();
        std::vector<Procedure> matchingProcedures;

        // Find SIDs for this airport
        for (auto it = procedures.begin(); it != procedures.end(); ++it) {
            if (it->second.icao == icao && it->second.type == PROCEDURE_SID) {
                // Check if any of this procedure's waypoints match the first route waypoint
                for (const auto& procWpt : it->second.waypoints) {
                    if (procWpt.getIdentifier() == firstWaypoint) {
                        matchingProcedures.push_back(it->second);
                        break;
                    }
                }
            }
        }

        // If no matches, just return the first active runway
        if (matchingProcedures.empty()) {
            return std::make_pair(depRunways[0], "");
        }

        // Try to find a procedure matching an active runway
        for (const auto& runway : depRunways) {
            for (const auto& proc : matchingProcedures) {
                if (proc.runway == runway) {
                    return std::make_pair(runway, proc.name);
                }
            }
        }

        // No match with active runways, return first active runway and first procedure
        return std::make_pair(depRunways[0], matchingProcedures[0].name);
    }

    std::optional<std::pair<std::string, std::string>> FindBestSTAR(
        const std::string& icao,
        const std::vector<std::string>& waypoints) const {

        if (waypoints.empty()) {
            return std::nullopt;
        }

        // Get active arrival runways
        auto arrRunways = GetArrivalRunways(icao);
        if (arrRunways.empty()) {
            return std::nullopt;
        }

        // Get the last waypoint in the route
        const auto& lastWaypoint = waypoints.back();

        // Get all procedures for this airport
        auto procedures = NavdataObject::GetProcedures();
        std::vector<Procedure> matchingProcedures;

        // Find STARs for this airport
        for (auto it = procedures.begin(); it != procedures.end(); ++it) {
            if (it->second.icao == icao && it->second.type == PROCEDURE_STAR) {
                // Check if any of this procedure's waypoints match the last route waypoint
                for (const auto& procWpt : it->second.waypoints) {
                    if (procWpt.getIdentifier() == lastWaypoint) {
                        matchingProcedures.push_back(it->second);
                        break;
                    }
                }
            }
        }

        // If no matches, just return the first active runway
        if (matchingProcedures.empty()) {
            return std::make_pair(arrRunways[0], "");
        }

        // Try to find a procedure matching an active runway
        for (const auto& runway : arrRunways) {
            for (const auto& proc : matchingProcedures) {
                if (proc.runway == runway) {
                    return std::make_pair(runway, proc.name);
                }
            }
        }

        // No match with active runways, return first active runway and first procedure
        return std::make_pair(arrRunways[0], matchingProcedures[0].name);
    }

private:
    // Store active runways per airport
    std::unordered_map<std::string, AirportRunways> runways_;
    mutable std::mutex airportRunwaysMutex_;
};
}