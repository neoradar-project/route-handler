#include "RunwayNetwork.h"
#include "Log.h"
#include <filesystem>

namespace RouteParser
{
    RunwayNetwork::RunwayNetwork(const std::string& dbPath, bool enableCache)
        : dbPath_(dbPath), useCache_(enableCache)
    {
        initialize(dbPath);
    }

    bool RunwayNetwork::initialize(const std::string& dbPath)
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

    bool RunwayNetwork::isValidDbPath(const std::string& path) noexcept
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
        catch (const std::exception& e)
        {
            Log::error("Error validating database path: {}", e.what());
            return false;
        }
    }

    bool RunwayNetwork::openDatabase()
    {
        try
        {
            db_ = std::make_unique<SQLite::Database>(dbPath_, SQLite::OPEN_READONLY);
            return true;
        }
        catch (const SQLite::Exception& e)
        {
            Log::error("Error opening runways database: {}", e.what());
            return false;
        }
        catch (const std::exception& e)
        {
            Log::error("Unexpected error opening database: {}", e.what());
            return false;
        }
    }

    bool RunwayNetwork::validateDatabase()
    {
        if (!db_)
            return false;

        try
        {
            SQLite::Statement query(*db_,
                "SELECT name FROM sqlite_master "
                "WHERE type='table' AND name='runways'");

            if (!query.executeStep())
            {
                Log::error("Required 'runways' table not found in database");
                return false;
            }

            SQLite::Statement schema(*db_, "PRAGMA table_info(runways)");
            std::vector<std::string> requiredColumns = {
                "airport_ref", "airport_ident", "length_ft", "width_ft",
                "surface", "lighted", "closed", "le_ident", "le_latitude_deg",
                "le_longitude_deg", "le_elevation_ft", "le_heading_degT",
                "le_displaced_threshold_ft", "he_ident", "he_latitude_deg",
                "he_longitude_deg", "he_elevation_ft", "he_heading_degT",
                "he_displaced_threshold_ft"
            };

            std::vector<std::string> foundColumns;
            while (schema.executeStep())
            {
                foundColumns.push_back(schema.getColumn(1).getText());
            }

            for (const auto& required : requiredColumns)
            {
                if (std::find(foundColumns.begin(), foundColumns.end(), required) == foundColumns.end())
                {
                    Log::error("Required column '{}' not found in runways table", required);
                    return false;
                }
            }

            return true;
        }
        catch (const SQLite::Exception& e)
        {
            Log::error("Error validating database schema: {}", e.what());
            return false;
        }
        catch (const std::exception& e)
        {
            Log::error("Unexpected error during database validation: {}", e.what());
            return false;
        }
    }

    std::optional<Runway> RunwayNetwork::findRunway(const std::string& id)
    {
        if (!isInitialized())
        {
            Log::error("Attempted to find runway with uninitialized database");
            return std::nullopt;
        }

        if (id.empty())
        {
            Log::error("Empty runway identifier provided");
            return std::nullopt;
        }

        // Runways are typically queried by airport, so we don't cache individual runways

        try
        {
            SQLite::Statement query(*db_,
                "SELECT * FROM runways WHERE id = ? LIMIT 1");

            query.bind(1, id);

            if (query.executeStep())
            {
                Runway runway = parseRunwayFromQuery(query);
                return runway;
            }
        }
        catch (const SQLite::Exception& e)
        {
            Log::error("Error querying runway {}: {}", id, e.what());
        }
        catch (const std::exception& e)
        {
            Log::error("Unexpected error querying runway {}: {}", id, e.what());
        }

        return std::nullopt;
    }

    Runway RunwayNetwork::parseRunwayFromQuery(SQLite::Statement& query)
    {
        std::string airport_ref = query.getColumn(1).getText();
        std::string airport_ident = query.getColumn(2).getText();
        double length_ft = query.getColumn(3).getDouble();
        double width_ft = query.getColumn(4).getDouble();
        std::string surface = query.getColumn(5).getText();
        bool lighted = (query.getColumn(6).getInt() == 1);
        bool closed = (query.getColumn(7).getInt() == 1);
        std::string le_ident = query.getColumn(8).getText();

        const std::string le_latitude = query.getColumn(9).getText();
        const std::string le_longitude = query.getColumn(10).getText();

        erkir::spherical::Point le_location;
        double lat = std::stod(le_latitude);
        double lon = std::stod(le_longitude);
        le_location = erkir::spherical::Point(lat, lon);

        double le_elevation_ft = query.getColumn(11).getDouble();
        double le_heading_deg = query.getColumn(12).getDouble();
        double le_displaced_threshold_ft = query.getColumn(13).getDouble();
        std::string he_ident = query.getColumn(14).getText();

        const std::string he_latitude = query.getColumn(15).getText();
        const std::string he_longitude = query.getColumn(16).getText();

        erkir::spherical::Point he_location;
        lat = std::stod(he_latitude);
        lon = std::stod(he_longitude);
        he_location = erkir::spherical::Point(lat, lon);

        double he_elevation_ft = query.getColumn(17).getDouble();
        double he_heading_deg = query.getColumn(18).getDouble();
        double he_displaced_threshold_ft = query.getColumn(19).getDouble();

        return Runway(
            airport_ref, airport_ident, length_ft, width_ft,
            surface, lighted, closed, le_ident, le_location,
            le_elevation_ft, le_heading_deg, le_displaced_threshold_ft,
            he_ident, he_location, he_elevation_ft, he_heading_deg,
            he_displaced_threshold_ft
        );
    }

    std::vector<Runway> RunwayNetwork::findRunwaysByAirport(const std::string& airportIdent)
    {
        std::vector<Runway> runways;

        if (!isInitialized())
        {
            Log::error("Attempted to find runways with uninitialized database");
            return runways;
        }

        if (airportIdent.empty())
        {
            Log::error("Empty airport identifier provided");
            return runways;
        }

        if (useCache_)
        {
            auto it = cache_.find(airportIdent);
            if (it != cache_.end())
            {
                return it->second;
            }
        }

        try
        {
            SQLite::Statement query(*db_,
                "SELECT * FROM runways WHERE airport_ident = ?");

            query.bind(1, airportIdent);

            while (query.executeStep())
            {
                try {
                    std::string airport_ref = query.getColumn(1).getText();
                    std::string airport_ident = query.getColumn(2).getText();
                    double length_ft = query.getColumn(3).getDouble();
                    double width_ft = query.getColumn(4).getDouble();
                    std::string surface = query.getColumn(5).getText();
                    bool lighted = (query.getColumn(6).getInt() == 1);
                    bool closed = (query.getColumn(7).getInt() == 1);
                    std::string le_ident = query.getColumn(8).getText();

                    const std::string le_latitude = query.getColumn(9).getText();
                    const std::string le_longitude = query.getColumn(10).getText();
                    if (le_latitude.empty() || le_longitude.empty()) {
                        Log::error("Invalid latitude or longitude for runway: {}", airport_ident);
                        continue;
                    }

                    erkir::spherical::Point le_location;
                    try {
                        double lat = std::stod(le_latitude);
                        double lon = std::stod(le_longitude);
                        le_location = erkir::spherical::Point(lat, lon);
                    }
                    catch (const std::invalid_argument& e) {
                        Log::error("Invalid latitude or longitude for runway: {}", airport_ident);
                        continue;
                    }

                    double le_elevation_ft = query.getColumn(11).getDouble();
                    double le_heading_deg = query.getColumn(12).getDouble();
                    double le_displaced_threshold_ft = query.getColumn(13).getDouble();
                    std::string he_ident = query.getColumn(14).getText();

                    const std::string he_latitude = query.getColumn(15).getText();
                    const std::string he_longitude = query.getColumn(16).getText();
                    if (he_latitude.empty() || he_longitude.empty()) {
                        Log::error("Invalid latitude or longitude for runway: {}", airport_ident);
                        continue;
                    }

                    erkir::spherical::Point he_location;
                    try {
                        double lat = std::stod(he_latitude);
                        double lon = std::stod(he_longitude);
                        he_location = erkir::spherical::Point(lat, lon);
                    }
                    catch (const std::invalid_argument& e) {
                        Log::error("Invalid latitude or longitude for runway: {}", airport_ident);
                        continue;
                    }

                    double he_elevation_ft = query.getColumn(17).getDouble();
                    double he_heading_deg = query.getColumn(18).getDouble();
                    double he_displaced_threshold_ft = query.getColumn(19).getDouble();

                    Runway runway(
                        airport_ref, airport_ident, length_ft, width_ft,
                        surface, lighted, closed, le_ident, le_location,
                        le_elevation_ft, le_heading_deg, le_displaced_threshold_ft,
                        he_ident, he_location, he_elevation_ft, he_heading_deg,
                        he_displaced_threshold_ft
                    );

                    runways.push_back(runway);
                }
                catch (const std::exception& e) {
                    Log::error("Error parsing runway data: {}", e.what());
                    continue;
                }
            }

            if (useCache_ && !runways.empty())
            {
                cache_[airportIdent] = runways;
            }
        }
        catch (const SQLite::Exception& e)
        {
            Log::error("Error querying runways for airport {}: {}", airportIdent, e.what());
        }
        catch (const std::exception& e)
        {
            Log::error("Unexpected error querying runways for airport {}: {}", airportIdent, e.what());
        }

        return runways;
    }

    bool RunwayNetwork::runwayExistsAtAirport(const std::string& airportIdent, const std::string& runwayIdent)
    {
        if (!isInitialized() || airportIdent.empty() || runwayIdent.empty())
        {
            return false;
        }

        auto runways = findRunwaysByAirport(airportIdent);
        for (const auto& runway : runways)
        {
            if (runway.getLeIdent() == runwayIdent || runway.getHeIdent() == runwayIdent)
            {
                return true;
            }
        }
        return false;
    }

    void RunwayNetwork::clearCache() noexcept
    {
        try
        {
            cache_.clear();
        }
        catch (const std::exception& e)
        {
            Log::error("Error clearing cache: {}", e.what());
        }
    }

} // namespace RouteParser