

#include "AirportNetwork.h"
#include "Log.h"
#include <filesystem>

namespace RouteParser
{

    AirportNetwork::AirportNetwork(const std::string &dbPath, bool enableCache)
        : dbPath_(dbPath), useCache_(enableCache)
    {
        initialize(dbPath);
    }

    bool AirportNetwork::initialize(const std::string &dbPath)
    {
        if (!dbPath.empty())
        {
            dbPath_ = dbPath;
        }

        if (!isValidDbPath(dbPath_))
        {
            Log::error("Invalid database path: {}", dbPath_);
            isInitialized_ = false;
            return false;
        }

        if (!openDatabase())
        {
            isInitialized_ = false;
            return false;
        }

        if (!validateDatabase())
        {
            Log::error("Database validation failed for: {}", dbPath_);
            db_.reset();
            isInitialized_ = false;
            return false;
        }

        isInitialized_ = true;
        return true;
    }

    bool AirportNetwork::isValidDbPath(const std::string &path) noexcept
    {
        if (path.empty())
        {
            return false;
        }

        try
        {
            std::filesystem::path dbPath(path);
            return std::filesystem::exists(dbPath) &&
                   std::filesystem::is_regular_file(dbPath) &&
                   dbPath.extension() == ".db";
        }
        catch (const std::exception &e)
        {
            Log::error("Error validating database path: {}", e.what());
            return false;
        }
    }

    bool AirportNetwork::openDatabase()
    {
        try
        {
            db_ = std::make_unique<SQLite::Database>(dbPath_, SQLite::OPEN_READONLY);
            return true;
        }
        catch (const SQLite::Exception &e)
        {
            Log::error("Error opening airports database: {}", e.what());
            return false;
        }
        catch (const std::exception &e)
        {
            Log::error("Unexpected error opening database: {}", e.what());
            return false;
        }
    }

    bool AirportNetwork::validateDatabase()
    {
        if (!db_)
            return false;

        try
        {
            SQLite::Statement query(*db_,
                                    "SELECT name FROM sqlite_master "
                                    "WHERE type='table' AND name='airports'");

            if (!query.executeStep())
            {
                Log::error("Required 'airports' table not found in database");
                return false;
            }

            SQLite::Statement schema(*db_, "PRAGMA table_info(airports)");
            std::vector<std::string> requiredColumns = {
                "ident", "name", "type", "latitude_deg",
                "longitude_deg", "elevation_ft", "iso_country", "iso_region"};

            std::vector<std::string> foundColumns;
            while (schema.executeStep())
            {
                foundColumns.push_back(schema.getColumn(1).getText());
            }

            for (const auto &required : requiredColumns)
            {
                if (std::find(foundColumns.begin(), foundColumns.end(), required) == foundColumns.end())
                {
                    Log::error("Required column '{}' not found in airports table", required);
                    return false;
                }
            }

            return true;
        }
        catch (const SQLite::Exception &e)
        {
            Log::error("Error validating database schema: {}", e.what());
            return false;
        }
        catch (const std::exception &e)
        {
            Log::error("Unexpected error during database validation: {}", e.what());
            return false;
        }
    }

    std::optional<Airport> AirportNetwork::findAirport(const std::string &ident)
    {
        if (!isInitialized())
        {
            Log::error("Attempted to find airport with uninitialized database");
            return std::nullopt;
        }

        if (ident.empty())
        {
            Log::error("Empty airport identifier provided");
            return std::nullopt;
        }

        if (useCache_)
        {
            auto it = cache_.find(ident);
            if (it != cache_.end())
            {
                return it->second;
            }
        }

        try
        {
            SQLite::Statement query(*db_,
                                    "SELECT ident, name, type, latitude_deg, longitude_deg, "
                                    "elevation_ft, iso_country, iso_region "
                                    "FROM airports WHERE ident = ? LIMIT 1");

            query.bind(1, ident);

            if (query.executeStep())
            {
                const std::string id = query.getColumn(0).getText();
                const std::string name = query.getColumn(1).getText();
                const std::string type = query.getColumn(2).getText();

                const double lat = query.getColumn(3).isNull() ? 0.0 : query.getColumn(3).getDouble();
                const double lon = query.getColumn(4).isNull() ? 0.0 : query.getColumn(4).getDouble();
                const int elevation = query.getColumn(5).isNull() ? 0 : query.getColumn(5).getInt();

                const std::string country = query.getColumn(6).getText();
                const std::string region = query.getColumn(7).getText();

                Airport airport(id, name, StringToAirportType(type),
                                erkir::spherical::Point(lat, lon),
                                elevation, country, region);

                if (useCache_)
                {
                    cache_[ident] = airport;
                }

                return airport;
            }
        }
        catch (const SQLite::Exception &e)
        {
            Log::error("Error querying airport {}: {}", ident, e.what());
        }
        catch (const std::exception &e)
        {
            Log::error("Unexpected error querying airport {}: {}", ident, e.what());
        }

        return std::nullopt;
    }

    void AirportNetwork::clearCache() noexcept
    {
        try
        {
            cache_.clear();
        }
        catch (const std::exception &e)
        {
            Log::error("Error clearing cache: {}", e.what());
        }
    }

} // namespace RouteParser