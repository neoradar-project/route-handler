#pragma once
#include "types/Airport.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <optional>
#include <unordered_map>

namespace RouteParser
{
    class AirportNetwork
    {
    public:
        explicit AirportNetwork(const std::string &dbPath, bool enableCache = true);

        std::optional<Airport> findAirport(const std::string &ident);
        void clearCache();

    private:
        bool initialize();

        std::string dbPath_;
        std::unique_ptr<SQLite::Database> db_;
        bool useCache_;
        std::unordered_map<std::string, Airport> cache_;
    };
}