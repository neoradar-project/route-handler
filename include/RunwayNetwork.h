#pragma once
#include "Runway.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace RouteParser
{
    class RunwayNetwork
    {
    public:
        RunwayNetwork(const std::string& dbPath, bool enableCache = true);

        bool initialize(const std::string& dbPath = "");
        bool isInitialized() const noexcept { return isInitialized_; }

        std::optional<Runway> findRunway(const std::string& id);

        std::vector<Runway> findRunwaysByAirport(const std::string& airportIdent);
        bool runwayExistsAtAirport(const std::string& airportIdent, const std::string& runwayIdent);

        void clearCache() noexcept;

    private:
        bool isValidDbPath(const std::string& path) noexcept;
        bool openDatabase();
        bool validateDatabase();
        Runway parseRunwayFromQuery(SQLite::Statement& query);

        std::string dbPath_;
        bool useCache_;
        bool isInitialized_ = false;
        std::unique_ptr<SQLite::Database> db_;

        std::unordered_map<std::string, std::vector<Runway>> cache_;
    };
}