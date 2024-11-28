#ifndef AIRWAYNETWORK_H
#define AIRWAYNETWORK_H

#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "types/Airway.h"
#include "types/ParsingError.h"

namespace RouteParser
{

    class NavdataObject;
    class AirwayNetwork
    {
    private:
        SQLite::Database db;

    public:
        AirwayNetwork(const std::string &dbPath);
        RouteValidationResult validateAirwayTraversal(
            const Waypoint &startFix,
            const std::string &airway,
            const std::string &endFix,
            int flightLevel,
            std::shared_ptr<NavdataObject> navdata);
        bool airwayExists(const std::string &airwayName);

    private:
        std::string join(const std::vector<std::string> &vec, const std::string &delimiter);
        bool isInitialized = false;
    };
}

#endif // AIRWAYNETWORK_H