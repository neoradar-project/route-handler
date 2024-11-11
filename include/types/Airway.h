#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <erkir/geo/sphericalpoint.h>
#include "exceptions/AirwayExceptions.h"
#include <iostream>

namespace RouteParser
{
    enum class AirwayLevel
    {
        BOTH, // 'B' - Both high and low
        HIGH, // 'H' - High level only
        LOW,  // 'L' - Low level only
        UNKNOWN
    };

    constexpr AirwayLevel stringToAirwayLevel(const char *level)
    {
        return (level[0] == 'B') ? AirwayLevel::BOTH : (level[0] == 'H') ? AirwayLevel::HIGH
                                                   : (level[0] == 'L')   ? AirwayLevel::LOW
                                                                         : AirwayLevel::UNKNOWN;
    }

    struct AirwayFix
    {
        std::string name;
        erkir::spherical::Point coord;

        bool operator==(const AirwayFix &other) const
        {
            return name == other.name;
        }
    };

    struct AirwaySegmentInfo
    {
        AirwayFix from;
        AirwayFix to;
        uint32_t minimum_level;
        bool canTraverse;
    };

    class Airway
    {
    private:
        struct Connection
        {
            size_t from_idx;
            size_t to_idx;
            uint32_t minimum_level;
            bool canTraverse;
        };

        // Basic airway data
        std::vector<AirwayFix> fixes;
        std::vector<AirwayFix> ordered_fixes;
        std::vector<Connection> connections;
        std::unordered_map<std::string, size_t> fix_indices;

        // Pre-computed paths cache
        // Key format: "fromFix:toFix"
        std::unordered_map<std::string, std::vector<AirwaySegmentInfo>> path_cache;

        // Helper methods
        size_t addFix(const AirwayFix &fix);
        void computeFixOrder();
        void precomputePaths();
        std::string makePathKey(const std::string &from, const std::string &to) const;
        std::vector<AirwaySegmentInfo> findPath(const std::string &from, const std::string &to) const;

    public:
        std::string name;
        AirwayLevel level{AirwayLevel::UNKNOWN};

        // Core functionality
        void setLevel(const std::string &levelChar);
        bool addSegment(const AirwayFix &from, const AirwayFix &to, uint32_t minLevel, bool canTraverse);
        void finalizeAirway();

        // Query methods
        std::vector<AirwaySegmentInfo> getSegmentsBetween(const std::string &from, const std::string &to) const;
        const std::vector<AirwayFix> &getAllFixes() const;
        const std::vector<AirwayFix> &getFixesInOrder() const;
        bool hasFix(const std::string &fixName) const;
    };

    class AirwayNetwork
    {
    private:
        std::unordered_multimap<std::string, std::shared_ptr<Airway>> airways;
        const Airway *findNearestAirway(const std::string &name, const erkir::spherical::Point &point) const;

    public:
        bool addAirwaySegment(const std::string &airwayName,
                              const std::string &levelChar,
                              const AirwayFix &from,
                              const AirwayFix &to,
                              uint32_t minLevel,
                              bool canTraverse);

        std::vector<AirwaySegmentInfo> getSegmentsBetween(const std::string &airwayName,
                                                          const std::string &from,
                                                          const std::string &to,
                                                          const erkir::spherical::Point &nearPoint) const;

        std::vector<AirwayFix> getFixesBetween(const std::string &airwayName,
                                               const std::string &from,
                                               const std::string &to,
                                               const erkir::spherical::Point &nearPoint) const;

        std::vector<std::string> getAllAirways() const;
        const Airway *getAirway(const std::string &name) const;
        const Airway *getAirwayByLatLon(const std::string &name,
                                        const erkir::spherical::Point &latLon) const;
        void finalizeAirways();
    };

} // namespace RouteParser