#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "types/Waypoint.h"
#include "erkir/geo/sphericalpoint.h"
#include "Log.h"
#include <optional>
#include "Utils.h"
namespace RouteParser
{

    class WaypointProvider
    {
    public:
        virtual ~WaypointProvider() = default;
        virtual std::vector<Waypoint> findWaypoint(const std::string &identifier) = 0;
        virtual bool initialize() = 0;
        virtual std::string getName() const = 0;
    };

    class AirwayWaypointProvider : public WaypointProvider
    {
    private:
        std::unique_ptr<SQLite::Database> db;
        std::string dbPath;
        std::string name;

    public:
        AirwayWaypointProvider(const std::string &path, const std::string &providerName)
            : dbPath(path), name(providerName) {}

        bool initialize() override
        {
            try
            {
                db = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READONLY);
                return true;
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error opening database: {}", e.what());
                return false;
            }
        }

        std::vector<Waypoint> findWaypoint(const std::string &identifier) override
        {
            std::vector<Waypoint> results;

            try
            {
                SQLite::Statement query(*db,
                                        "SELECT identifier, latitude, longitude FROM waypoints WHERE identifier = ?");
                query.bind(1, identifier);

                while (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    double lat = query.getColumn(1).getDouble();
                    double lon = query.getColumn(2).getDouble();

                    results.emplace_back(Utils::GetWaypointTypeByIdentifier(id), id, erkir::spherical::Point(lat, lon));
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error querying waypoint: {}", e.what());
            }

            return results;
        }

        std::string getName() const override
        {
            return name;
        }
    };

    class NavdataWaypointProvider : public WaypointProvider
    {
    private:
        std::unique_ptr<SQLite::Database> db;
        std::string dbPath;
        std::string name;

    public:
        NavdataWaypointProvider(const std::string &path, const std::string &providerName)
            : dbPath(path), name(providerName) {}

        bool initialize() override
        {
            try
            {
                db = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READONLY);
                return true;
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error opening database: {}", e.what());
                return false;
            }
        }

        std::vector<Waypoint> findWaypoint(const std::string &identifier) override
        {
            std::vector<Waypoint> results;

            try
            {
                SQLite::Statement query(*db,
                                        "SELECT ident, type, frequency_khz, latitude_deg, longitude_deg FROM waypoints WHERE identifier = ?");
                query.bind(1, identifier);

                while (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    std::string type = query.getColumn(1).getText();
                    int frequency = query.getColumn(2).getInt();
                    double lat = query.getColumn(3).getDouble();
                    double lon = query.getColumn(4).getDouble();

                    results.emplace_back(Utils::GetWaypointTypeByIdentifier(id), id, erkir::spherical::Point(lat, lon));
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error querying waypoint: {}", e.what());
            }

            return results;
        }

        std::string getName() const override
        {
            return name;
        }
    };

    class WaypointNetwork
    {
    private:
        std::vector<std::unique_ptr<WaypointProvider>> providers;
        std::unordered_multimap<std::string, Waypoint> cache;
        bool useCache;

    public:
        WaypointNetwork(bool enableCache = true) : useCache(enableCache) {}

        void addProvider(std::unique_ptr<WaypointProvider> provider)
        {
            if (provider->initialize())
            {
                providers.push_back(std::move(provider));
            }
        }

        void initialCache(std::unordered_multimap<std::string, Waypoint> initialCache)
        {
            cache = initialCache;
        }

        // Modified findWaypoint to work with multimap
        std::vector<Waypoint> findWaypoint(const std::string &identifier)
        {
            // Check cache first if enabled
            if (useCache)
            {
                auto range = cache.equal_range(identifier);
                if (range.first != range.second)
                {
                    std::vector<Waypoint> results;
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        results.push_back(it->second);
                    }
                    return results;
                }
            }

            std::vector<Waypoint> results;

            // Try each provider in order
            for (const auto &provider : providers)
            {

                auto providerResults = provider->findWaypoint(identifier);
                if (!providerResults.empty())
                {
                    Log::debug("Found {} in {}", identifier, provider->getName());
                    results.insert(results.end(), providerResults.begin(), providerResults.end());
                }
            }

            if (useCache && !results.empty())
            {
                for (const auto &waypoint : results)
                {
                    cache.insert({identifier, waypoint});
                }
            }

            return results;
        }

        void clearCache()
        {
            cache.clear();
        }

        // The new findWaypoint with position reference
        std::vector<Waypoint> findWaypoint(const std::string &identifier,
                                           const erkir::spherical::Point &referencePoint)
        {
            // Get all waypoints first
            auto waypoints = findWaypoint(identifier);

            // Create multimap to sort by distance
            std::multimap<double, Waypoint> waypointsByDistance;

            // Sort waypoints by distance
            for (const auto &waypoint : waypoints)
            {
                double distance = referencePoint.distanceTo(waypoint.getPosition());
                waypointsByDistance.insert({distance, waypoint});
            }

            // Convert sorted map to vector
            std::vector<Waypoint> results;
            std::transform(waypointsByDistance.begin(), waypointsByDistance.end(),
                           std::back_inserter(results),
                           [](const auto &pair)
                           { return pair.second; });

            return results;
        }

        // Helper method to get just the closest waypoint
        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier,
                                                    const erkir::spherical::Point &referencePoint)
        {
            auto waypoints = findWaypoint(identifier, referencePoint);
            if (waypoints.empty())
            {
                return std::nullopt;
            }
            return waypoints[0]; // First waypoint is the closest one
        }

        // Helper method to get first waypoint
        std::optional<Waypoint> findFirstWaypoint(const std::string &identifier)
        {
            auto waypoints = findWaypoint(identifier);
            if (waypoints.empty())
            {
                return std::nullopt;
            }
            return waypoints[0];
        }

        std::optional<Waypoint> findClosestWaypointTo(const std::string &nextWaypoint,
                                                      std::optional<Waypoint> reference = std::nullopt)
        {
            // If no reference provided, try to find one from the waypoint name
            if (!reference.has_value())
            {
                reference = findFirstWaypoint(nextWaypoint);
                if (!reference.has_value())
                {
                    return std::nullopt;
                }
            }

            // Get all waypoints matching the identifier
            auto waypoints = findWaypoint(nextWaypoint);
            if (waypoints.empty())
            {
                return std::nullopt;
            }

            // Sort waypoints by distance to reference
            std::multimap<double, Waypoint> waypointsByDistance;
            for (const auto &waypoint : waypoints)
            {
                double distance = reference->getPosition().distanceTo(waypoint.getPosition());
                waypointsByDistance.insert({distance, waypoint});
            }

            if (!waypointsByDistance.empty())
            {
                return waypointsByDistance.begin()->second; // Return closest waypoint
            }

            return std::nullopt;
        }
    };

} // namespace RouteParser