// AirportNetwork.cpp
#include "AirportNetwork.h"
#include "Log.h"

namespace RouteParser
{
    AirportNetwork::AirportNetwork(const std::string &dbPath, bool enableCache)
        : dbPath_(dbPath), useCache_(enableCache)
    {
        initialize();
    }

    bool AirportNetwork::initialize()
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
    }

    std::optional<Airport> AirportNetwork::findAirport(const std::string &ident)
    {
        // Check cache first if enabled
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
                                    "SELECT ident, name, type, latitude_deg, longitude_deg, elevation_ft, iso_country, iso_region "
                                    "FROM airports WHERE ident = ? LIMIT 1");

            query.bind(1, ident);

            if (query.executeStep())
            {
                std::string id = query.getColumn(0).getText();
                std::string name = query.getColumn(1).getText();
                std::string type = query.getColumn(2).getText();
                double lat = query.getColumn(3).getDouble();
                double lon = query.getColumn(4).getDouble();
                int elevation = query.getColumn(5).getInt();
                std::string country = query.getColumn(6).getText();
                std::string region = query.getColumn(7).getText();
                Airport airport(id, name, StringToAirportType(type),
                                erkir::spherical::Point(lat, lon),
                                elevation, country, region);

                // Add to cache if enabled
                if (useCache_)
                {
                    cache_[ident] = airport;
                }

                return airport;
            }
        }
        catch (const SQLite::Exception &e)
        {
            Log::error("Error querying airport: {}", e.what());
        }

        return std::nullopt;
    }

    void AirportNetwork::clearCache()
    {
        cache_.clear();
    }
}