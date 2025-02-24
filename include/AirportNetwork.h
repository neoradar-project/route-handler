#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <SQLiteCpp/SQLiteCpp.h>
#include "types/Airport.h"

namespace RouteParser
{

    class AirportNetwork
    {
    public:
        explicit AirportNetwork(const std::string &dbPath = "", bool enableCache = true);
        ~AirportNetwork() = default;

        AirportNetwork(const AirportNetwork &) = delete;
        AirportNetwork &operator=(const AirportNetwork &) = delete;

        AirportNetwork(AirportNetwork &&) noexcept = default;
        AirportNetwork &operator=(AirportNetwork &&) noexcept = default;

        [[nodiscard]] bool isInitialized() const noexcept { return db_ != nullptr && isInitialized_; }

        [[nodiscard]] std::optional<Airport> findAirport(const std::string &ident);

        void clearCache() noexcept;

        bool initialize(const std::string &dbPath = "");

    private:
        bool openDatabase();
        bool validateDatabase();
        static bool isValidDbPath(const std::string &path) noexcept;

        std::string dbPath_;
        bool useCache_;
        bool isInitialized_{false};
        std::unique_ptr<SQLite::Database> db_;
        std::unordered_map<std::string, Airport> cache_;
    };

} // namespace RouteParser