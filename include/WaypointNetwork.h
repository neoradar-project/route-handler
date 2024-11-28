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
        virtual std::optional<Waypoint> findClosestWaypoint(const std::string &identifier, const erkir::spherical::Point &reference) = 0;
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
                SQLite::Statement query(*db, "SELECT identifier, latitude, longitude FROM waypoints WHERE identifier = ?");
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

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier, const erkir::spherical::Point &reference) override
        {
            try
            {
                SQLite::Statement query(*db,
                                        "SELECT identifier, latitude, longitude, "
                                        "(6371 * acos(cos(radians(?)) * cos(radians(latitude)) * cos(radians(longitude) - radians(?)) + sin(radians(?)) * sin(radians(latitude)))) AS distance "
                                        "FROM waypoints "
                                        "WHERE identifier = ? "
                                        "ORDER BY distance ASC LIMIT 1");

                double lat = reference.latitude().degrees();  // Convert to double
                double lon = reference.longitude().degrees(); // Convert to double

                query.bind(1, lat);
                query.bind(2, lon);
                query.bind(3, lat);
                query.bind(4, identifier);

                if (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    double lat = query.getColumn(1).getDouble();
                    double lon = query.getColumn(2).getDouble();
                    return Waypoint(Utils::GetWaypointTypeByIdentifier(id), id, erkir::spherical::Point(lat, lon));
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error querying closest waypoint: {}", e.what());
            }
            return std::nullopt;
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
                                        "SELECT ident, type, frequency_khz, latitude_deg, longitude_deg "
                                        "FROM navaids WHERE ident = ?");
                query.bind(1, identifier);

                while (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    std::string type = query.getColumn(1).getText();
                    int frequency = query.getColumn(2).getInt();
                    double lat = query.getColumn(3).getDouble();
                    double lon = query.getColumn(4).getDouble();
                    results.emplace_back(Utils::GetWaypointTypeByTypeString(id), id,
                                         erkir::spherical::Point(lat, lon), frequency * 1000);
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error querying waypoint: {}", e.what());
            }
            return results;
        }

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier, const erkir::spherical::Point &reference) override
        {
            try
            {
                SQLite::Statement query(*db,
                                        "SELECT ident, type, frequency_khz, latitude_deg, longitude_deg, "
                                        "(6371 * acos(cos(radians(?)) * cos(radians(latitude_deg)) * cos(radians(longitude_deg) - radians(?)) + sin(radians(?)) * sin(radians(latitude_deg)))) AS distance "
                                        "FROM navaids "
                                        "WHERE ident = ? "
                                        "ORDER BY distance ASC LIMIT 1");

                double lat = reference.latitude().degrees();  // Convert to double
                double lon = reference.longitude().degrees(); // Convert to double

                query.bind(1, lat);
                query.bind(2, lon);
                query.bind(3, lat);
                query.bind(4, identifier);

                if (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    std::string type = query.getColumn(1).getText();
                    int frequency = query.getColumn(2).getInt();
                    double lat = query.getColumn(3).getDouble();
                    double lon = query.getColumn(4).getDouble();
                    return Waypoint(Utils::GetWaypointTypeByTypeString(id), id,
                                    erkir::spherical::Point(lat, lon), frequency * 1000);
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("Error querying closest waypoint: {}", e.what());
            }
            return std::nullopt;
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
            cache = std::move(initialCache);
        }

        // Original findWaypoint with early stopping
        std::vector<Waypoint> findWaypoint(const std::string &identifier)
        {
            // Check cache first
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

            for (const auto &provider : providers)
            {
                auto providerResults = provider->findWaypoint(identifier);
                if (!providerResults.empty())
                {
                    if (useCache)
                    {
                        for (const auto &waypoint : providerResults)
                        {
                            cache.insert({identifier, waypoint});
                        }
                    }
                    return providerResults; // Early return once we find results
                }
            }

            return {};
        }

        // Original findFirstWaypoint implementation
        std::optional<Waypoint> findFirstWaypoint(const std::string &identifier)
        {
            auto waypoints = findWaypoint(identifier);
            if (waypoints.empty())
            {
                return std::nullopt;
            }
            return waypoints[0];
        }

        // Original findClosestWaypointTo implementation
        std::optional<Waypoint> findClosestWaypointTo(const std::string &identifier, std::optional<Waypoint> reference = std::nullopt)
        {
            // If no reference provided, try to find one from the waypoint name
            if (!reference.has_value())
            {
                reference = findFirstWaypoint(identifier);
                if (!reference.has_value())
                {
                    return std::nullopt;
                }
            }

            // Get all waypoints matching the identifier
            auto waypoints = findWaypoint(identifier);
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

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier,
                                                    const erkir::spherical::Point &referencePoint)
        {
            auto waypoints = findWaypoint(identifier);
            if (waypoints.empty())
            {
                return std::nullopt;
            }

            // Find closest waypoint
            auto closest = std::min_element(waypoints.begin(), waypoints.end(),
                                            [&referencePoint](const Waypoint &a, const Waypoint &b)
                                            {
                                                return referencePoint.distanceTo(a.getPosition()) <
                                                       referencePoint.distanceTo(b.getPosition());
                                            });

            return *closest;
        }

        void clearCache()
        {
            cache.clear();
        }
    };
}