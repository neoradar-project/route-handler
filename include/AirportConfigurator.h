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
        airportRunways_ = airportRunways;
    }

    inline std::vector<std::string> GetDepRunways(const std::string& icao) const
    {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        if (auto it = airportRunways_.find(icao); it != airportRunways_.end()) {
            return it->second.depRunways;
        }
        return {};
    }

    inline std::vector<std::string> GetArrRunways(const std::string& icao) const
    {
        std::lock_guard<std::mutex> lock(airportRunwaysMutex_);
        if (auto it = airportRunways_.find(icao); it != airportRunways_.end()) {
            return it->second.arrRunways;
        }
        return {};
    }

private:
    // Store active runways per airport
    std::unordered_map<std::string, AirportRunways> airportRunways_;
    mutable std::mutex airportRunwaysMutex_;
};
}