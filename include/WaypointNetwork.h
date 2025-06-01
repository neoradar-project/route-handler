#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
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
        virtual bool isInitialized() const = 0;
        virtual std::string getName() const = 0;
    };

    class NseWaypointProvider : public WaypointProvider {
    private:
        std::unordered_map<std::string, std::vector<Waypoint>> waypointsByIdentifier;
        std::string name;
        bool initialized { false };

    public:
        NseWaypointProvider(
            const std::vector<Waypoint>& waypoints, const std::string& providerName)
            : name(providerName)
        {

            // Organize waypoints by identifier for quick lookup
            for (const auto& waypoint : waypoints) {
                waypointsByIdentifier[waypoint.getIdentifier()].push_back(waypoint);
            }

            Log::info("[{}] Constructed with {} unique waypoint identifiers", name,
                waypointsByIdentifier.size());
        }

        std::vector<Waypoint> findWaypoint(const std::string& identifier) override
        {
            if (!isInitialized()) {
                Log::error(
                    "[{}] Attempted to find waypoint with uninitialized provider", name);
                return {};
            }

            if (identifier.empty()) {
                Log::error("[{}] Empty waypoint identifier provided", name);
                return {};
            }

            auto it = waypointsByIdentifier.find(identifier);
            if (it != waypointsByIdentifier.end()) {
                return it->second;
            }

            return {};
        }

        std::optional<Waypoint> findClosestWaypoint(const std::string& identifier,
            const erkir::spherical::Point& reference) override
        {

            if (!isInitialized()) {
                Log::error(
                    "[{}] Attempted to find closest waypoint with uninitialized provider",
                    name);
                return std::nullopt;
            }

            if (identifier.empty()) {
                Log::error(
                    "[{}] Empty waypoint identifier provided for closest search", name);
                return std::nullopt;
            }

            auto waypoints = findWaypoint(identifier);
            if (waypoints.empty()) {
                return std::nullopt;
            }

            double minDistance = std::numeric_limits<double>::max();
            std::optional<Waypoint> closest;

            for (const auto& waypoint : waypoints) {
                double distance = reference.distanceTo(waypoint.getPosition());
                if (distance < minDistance) {
                    minDistance = distance;
                    closest = waypoint;
                }
            }

            return closest;
        }

        bool initialize() override
        {
            Log::info("[{}] Initializing NSE waypoint provider with {} unique waypoint "
                      "identifiers",
                name, waypointsByIdentifier.size());
            initialized = true;
            return true;
        }

        bool isInitialized() const override { return initialized; }

        std::string getName() const override { return name; }
    };

    class BaseWaypointProvider : public WaypointProvider
    {
    protected:
        std::unique_ptr<SQLite::Database> db;
        std::string dbPath;
        std::string name;
        bool initialized{false};

        bool isValidDbPath() const noexcept
        {
            if (dbPath.empty())
            {
                return false;
            }

            try
            {
                std::filesystem::path path(dbPath);
                return std::filesystem::exists(path) &&
                       std::filesystem::is_regular_file(path);
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Error validating database path: {}", name, e.what());
                return false;
            }
        }

        virtual bool validateDatabase()
        {
            return true; // Default implementation - override in derived classes
        }

    public:
        BaseWaypointProvider(const std::string &path, const std::string &providerName)
            : dbPath(path), name(providerName) {}

        bool isInitialized() const override
        {
            return initialized && db != nullptr;
        }

        std::string getName() const override
        {
            return name;
        }

        bool initialize() override
        {
            if (!isValidDbPath())
            {
                Log::error("[{}] Invalid database path: {}", name, dbPath);
                initialized = false;
                return false;
            }

            try
            {
                db = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READONLY);

                if (!validateDatabase())
                {
                    Log::error("[{}] Database validation failed for: {}", name, dbPath);
                    db.reset();
                    initialized = false;
                    return false;
                }

                initialized = true;
                return true;
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error opening database: {}", name, e.what());
                initialized = false;
                return false;
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error opening database: {}", name, e.what());
                initialized = false;
                return false;
            }
        }
    };

    class AirwayWaypointProvider : public BaseWaypointProvider
    {
    protected:
        bool validateDatabase() override
        {
            if (!db)
                return false;

            try
            {
                // Check if the required table exists
                SQLite::Statement tableCheck(*db,
                                             "SELECT name FROM sqlite_master WHERE type='table' AND name='waypoints'");

                if (!tableCheck.executeStep())
                {
                    Log::error("[{}] Required 'waypoints' table not found", name);
                    return false;
                }

                // Check columns
                SQLite::Statement columnCheck(*db, "PRAGMA table_info(waypoints)");
                std::vector<std::string> requiredColumns = {"identifier", "latitude", "longitude"};
                std::vector<std::string> foundColumns;

                while (columnCheck.executeStep())
                {
                    foundColumns.push_back(columnCheck.getColumn(1).getText());
                }

                for (const auto &required : requiredColumns)
                {
                    if (std::find(foundColumns.begin(), foundColumns.end(), required) == foundColumns.end())
                    {
                        Log::error("[{}] Required column '{}' not found in waypoints table", name, required);
                        return false;
                    }
                }

                return true;
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error validating database schema: {}", name, e.what());
                return false;
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error during database validation: {}", name, e.what());
                return false;
            }
        }

    public:
        AirwayWaypointProvider(const std::string &path, const std::string &providerName)
            : BaseWaypointProvider(path, providerName) {}

        std::vector<Waypoint> findWaypoint(const std::string &identifier) override
        {
            std::vector<Waypoint> results;

            if (!isInitialized())
            {
                Log::error("[{}] Attempted to find waypoint with uninitialized database", name);
                return results;
            }

            if (identifier.empty())
            {
                Log::error("[{}] Empty waypoint identifier provided", name);
                return results;
            }

            try
            {
                SQLite::Statement query(*db, "SELECT identifier, latitude, longitude FROM waypoints WHERE identifier = ?");
                query.bind(1, identifier);

                while (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    double lat = query.getColumn(1).isNull() ? 0.0 : query.getColumn(1).getDouble();
                    double lon = query.getColumn(2).isNull() ? 0.0 : query.getColumn(2).getDouble();

                    results.emplace_back(Utils::GetWaypointTypeByIdentifier(id), id, id,  erkir::spherical::Point(lat, lon));
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error querying waypoint {}: {}", name, identifier, e.what());
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error querying waypoint {}: {}", name, identifier, e.what());
            }

            return results;
        }

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier, const erkir::spherical::Point &reference) override
        {
            if (!isInitialized())
            {
                Log::error("[{}] Attempted to find closest waypoint with uninitialized database", name);
                return std::nullopt;
            }

            if (identifier.empty())
            {
                Log::error("[{}] Empty waypoint identifier provided for closest search", name);
                return std::nullopt;
            }

            try
            {
                SQLite::Statement query(*db,
                                        "SELECT identifier, latitude, longitude, "
                                        "(6371 * acos(cos(radians(?)) * cos(radians(latitude)) * cos(radians(longitude) - radians(?)) + sin(radians(?)) * sin(radians(latitude)))) AS distance "
                                        "FROM waypoints "
                                        "WHERE identifier = ? "
                                        "ORDER BY distance ASC LIMIT 1");

                double lat = reference.latitude().degrees();
                double lon = reference.longitude().degrees();

                query.bind(1, lat);
                query.bind(2, lon);
                query.bind(3, lat);
                query.bind(4, identifier);

                if (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    double lat = query.getColumn(1).isNull() ? 0.0 : query.getColumn(1).getDouble();
                    double lon = query.getColumn(2).isNull() ? 0.0 : query.getColumn(2).getDouble();

                    return Waypoint(Utils::GetWaypointTypeByIdentifier(id), id,id,  erkir::spherical::Point(lat, lon));
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error querying closest waypoint {}: {}", name, identifier, e.what());
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error querying closest waypoint {}: {}", name, identifier, e.what());
            }

            return std::nullopt;
        }
    };

    class NavdataWaypointProvider : public BaseWaypointProvider
    {
    protected:
        bool validateDatabase() override
        {
            if (!db)
                return false;

            try
            {
                // Check if the required table exists
                SQLite::Statement tableCheck(*db,
                                             "SELECT name FROM sqlite_master WHERE type='table' AND name='navaids'");

                if (!tableCheck.executeStep())
                {
                    Log::error("[{}] Required 'navaids' table not found", name);
                    return false;
                }

                // Check columns
                SQLite::Statement columnCheck(*db, "PRAGMA table_info(navaids)");
                std::vector<std::string> requiredColumns = {
                    "ident", "type", "frequency_khz", "latitude_deg", "longitude_deg"};
                std::vector<std::string> foundColumns;

                while (columnCheck.executeStep())
                {
                    foundColumns.push_back(columnCheck.getColumn(1).getText());
                }

                for (const auto &required : requiredColumns)
                {
                    if (std::find(foundColumns.begin(), foundColumns.end(), required) == foundColumns.end())
                    {
                        Log::error("[{}] Required column '{}' not found in navaids table", name, required);
                        return false;
                    }
                }

                return true;
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error validating database schema: {}", name, e.what());
                return false;
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error during database validation: {}", name, e.what());
                return false;
            }
        }

    public:
        NavdataWaypointProvider(const std::string &path, const std::string &providerName)
            : BaseWaypointProvider(path, providerName) {}

        std::vector<Waypoint> findWaypoint(const std::string &identifier) override
        {
            std::vector<Waypoint> results;

            if (!isInitialized())
            {
                Log::error("[{}] Attempted to find waypoint with uninitialized database", name);
                return results;
            }

            if (identifier.empty())
            {
                Log::error("[{}] Empty waypoint identifier provided", name);
                return results;
            }

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
                    int frequency = query.getColumn(2).isNull() ? 0 : query.getColumn(2).getInt();
                    double lat = query.getColumn(3).isNull() ? 0.0 : query.getColumn(3).getDouble();
                    double lon = query.getColumn(4).isNull() ? 0.0 : query.getColumn(4).getDouble();

                    results.emplace_back(Utils::GetWaypointTypeByTypeString(id), id,id,
                                         erkir::spherical::Point(lat, lon), frequency * 1000);
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error querying waypoint {}: {}", name, identifier, e.what());
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error querying waypoint {}: {}", name, identifier, e.what());
            }

            return results;
        }

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier, const erkir::spherical::Point &reference) override
        {
            if (!isInitialized())
            {
                Log::error("[{}] Attempted to find closest waypoint with uninitialized database", name);
                return std::nullopt;
            }

            if (identifier.empty())
            {
                Log::error("[{}] Empty waypoint identifier provided for closest search", name);
                return std::nullopt;
            }

            try
            {
                SQLite::Statement query(*db,
                                        "SELECT ident, type, frequency_khz, latitude_deg, longitude_deg, "
                                        "(6371 * acos(cos(radians(?)) * cos(radians(latitude_deg)) * cos(radians(longitude_deg) - radians(?)) + sin(radians(?)) * sin(radians(latitude_deg)))) AS distance "
                                        "FROM navaids "
                                        "WHERE ident = ? "
                                        "ORDER BY distance ASC LIMIT 1");

                double lat = reference.latitude().degrees();
                double lon = reference.longitude().degrees();

                query.bind(1, lat);
                query.bind(2, lon);
                query.bind(3, lat);
                query.bind(4, identifier);

                if (query.executeStep())
                {
                    std::string id = query.getColumn(0).getText();
                    std::string type = query.getColumn(1).getText();
                    int frequency = query.getColumn(2).isNull() ? 0 : query.getColumn(2).getInt();
                    double lat = query.getColumn(3).isNull() ? 0.0 : query.getColumn(3).getDouble();
                    double lon = query.getColumn(4).isNull() ? 0.0 : query.getColumn(4).getDouble();

                    return Waypoint(Utils::GetWaypointTypeByTypeString(id), id,id,
                                    erkir::spherical::Point(lat, lon), frequency * 1000);
                }
            }
            catch (const SQLite::Exception &e)
            {
                Log::error("[{}] Error querying closest waypoint {}: {}", name, identifier, e.what());
            }
            catch (const std::exception &e)
            {
                Log::error("[{}] Unexpected error querying closest waypoint {}: {}", name, identifier, e.what());
            }

            return std::nullopt;
        }
    };

    class WaypointNetwork
    {
    private:
        std::vector<std::unique_ptr<WaypointProvider>> providers;
        std::unordered_multimap<std::string, Waypoint> cache;
        bool useCache;
        bool initialized{false};

    public:
        WaypointNetwork(bool enableCache = true) : useCache(enableCache) {}

        bool isInitialized() const
        {
            return initialized && !providers.empty();
        }

        bool addProvider(std::unique_ptr<WaypointProvider> provider)
        {
            if (!provider)
            {
                Log::error("Attempted to add null provider to WaypointNetwork");
                return false;
            }

            try
            {
                if (provider->initialize())
                {
                    Log::info("Successfully initialized waypoint provider: {}", provider->getName());
                    providers.push_back(std::move(provider));
                    initialized = true;
                    return true;
                }
                else
                {
                    Log::warn("Failed to initialize waypoint provider: {}", provider->getName());
                    return false;
                }
            }
            catch (const std::exception &e)
            {
                Log::error("Exception adding provider {}: {}",
                           provider->getName(), e.what());
                return false;
            }
        }

        void initialCache(std::unordered_multimap<std::string, Waypoint> initialCache)
        {
            try
            {
                cache = std::move(initialCache);
            }
            catch (const std::exception &e)
            {
                Log::error("Error initializing cache: {}", e.what());
                cache.clear();
            }
        }

        std::vector<Waypoint> findWaypoint(const std::string &identifier)
        {
            if (!isInitialized())
            {
                Log::error("Attempted to find waypoint with no initialized providers");
                return {};
            }

            if (identifier.empty())
            {
                Log::error("Empty waypoint identifier provided");
                return {};
            }

            try
            {
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
                    if (!provider->isInitialized())
                    {
                        Log::warn("Skipping uninitialized provider: {}", provider->getName());
                        continue;
                    }

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
                        return providerResults;
                    }
                }
            }
            catch (const std::exception &e)
            {
                Log::error("Error finding waypoint {}: {}", identifier, e.what());
            }

            return {};
        }

        std::optional<Waypoint> findFirstWaypoint(const std::string &identifier)
        {
            if (!isInitialized())
            {
                Log::error("Attempted to find first waypoint with no initialized providers");
                return std::nullopt;
            }

            if (identifier.empty())
            {
                Log::error("Empty waypoint identifier provided");
                return std::nullopt;
            }

            try
            {
                auto waypoints = findWaypoint(identifier);
                if (waypoints.empty())
                {
                    return std::nullopt;
                }
                return waypoints[0];
            }
            catch (const std::exception &e)
            {
                Log::error("Error finding first waypoint {}: {}", identifier, e.what());
                return std::nullopt;
            }
        }

        std::optional<Waypoint> findClosestWaypointTo(const std::string &identifier,
                                                      std::optional<Waypoint> reference = std::nullopt)
        {
            if (!isInitialized())
            {
                Log::error("Attempted to find closest waypoint with no initialized providers");
                return std::nullopt;
            }

            if (identifier.empty())
            {
                Log::error("Empty waypoint identifier provided for closest search");
                return std::nullopt;
            }

            try
            {
                if (!reference.has_value())
                {
                    reference = findFirstWaypoint(identifier);
                    if (!reference.has_value())
                    {
                        return std::nullopt;
                    }
                }

                auto waypoints = findWaypoint(identifier);
                if (waypoints.empty())
                {
                    return std::nullopt;
                }

                std::multimap<double, Waypoint> waypointsByDistance;
                for (const auto &waypoint : waypoints)
                {
                    double distance = reference->getPosition().distanceTo(waypoint.getPosition());
                    waypointsByDistance.insert({distance, waypoint});
                }

                if (!waypointsByDistance.empty())
                {
                    return waypointsByDistance.begin()->second;
                }
            }
            catch (const std::exception &e)
            {
                Log::error("Error finding closest waypoint to {}: {}", identifier, e.what());
            }

            return std::nullopt;
        }

        std::optional<Waypoint> findClosestWaypoint(const std::string &identifier,
                                                    const erkir::spherical::Point &referencePoint)
        {
            if (!isInitialized())
            {
                Log::error("Attempted to find closest waypoint with no initialized providers");
                return std::nullopt;
            }

            if (identifier.empty())
            {
                Log::error("Empty waypoint identifier provided for closest search");
                return std::nullopt;
            }

            try
            {
                auto waypoints = findWaypoint(identifier);
                if (waypoints.empty())
                {
                    return std::nullopt;
                }

                auto closest = std::min_element(waypoints.begin(), waypoints.end(),
                                                [&referencePoint](const Waypoint &a, const Waypoint &b)
                                                {
                                                    return referencePoint.distanceTo(a.getPosition()) <
                                                           referencePoint.distanceTo(b.getPosition());
                                                });

                return *closest;
            }
            catch (const std::exception &e)
            {
                Log::error("Error finding closest waypoint {}: {}", identifier, e.what());
                return std::nullopt;
            }
        }

        void clearCache()
        {
            try
            {
                cache.clear();
            }
            catch (const std::exception &e)
            {
                Log::error("Error clearing cache: {}", e.what());
            }
        }
    };
}