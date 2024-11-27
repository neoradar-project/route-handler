#include "AirwayNetwork.h"
#include <chrono>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <functional>
#include "Navdata.h"

namespace RouteParser
{
    AirwayNetwork::AirwayNetwork(const std::string &dbPath) : db(dbPath, SQLite::OPEN_READONLY) {}

    struct AirwayGraph
    {
        std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
        std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> levels;
    };

    RouteValidationResult AirwayNetwork::validateAirwayTraversal(const Waypoint &startFix, const std::string &airway, const std::string &endFix, int flightLevel)
    {
        RouteValidationResult result;
        result.isValid = true;

        try
        {
            // Verify airway exists
            if (!airwayExists(airway))
            {
                result.isValid = false;
                result.errors.push_back({UNKNOWN_AIRWAY, "Airway not found: " + airway, 0, "", ERROR});
                return result;
            }

            // Verify end fix exists
            auto endFixWaypoint = NavdataObject::FindClosestWaypointTo(endFix, startFix);
            if (!endFixWaypoint)
            {
                result.isValid = false;
                result.errors.push_back({UNKNOWN_WAYPOINT, "End fix not found: " + endFix, 0, "", ERROR});
                return result;
            }

            // Build graph from database
            AirwayGraph graph;
            SQLite::Statement segStmt(db, R"(
                SELECT 
                    ds.from_identifier,
                    ds.to_identifier,
                    ds.minimum_level
                FROM direct_segments ds
                WHERE ds.airway_name = ? 
                AND ds.can_traverse = 1
                ORDER BY ds.rowid
            )");
            segStmt.bind(1, airway);

            while (segStmt.executeStep())
            {
                std::string fromId = segStmt.getColumn(0).getText();
                std::string toId = segStmt.getColumn(1).getText();
                uint32_t minLevel = segStmt.getColumn(2).getUInt();

                graph.adjacencyList[fromId].push_back(toId);
                graph.levels[fromId][toId] = minLevel;
            }

            // DFS implementation
            std::function<bool(const std::string &, std::unordered_set<std::string> &, std::vector<std::string> &)> dfs;
            dfs = [&](const std::string &current, std::unordered_set<std::string> &visited, std::vector<std::string> &path) -> bool
            {
                if (current == endFix)
                {
                    return true;
                }

                if (visited.find(current) != visited.end())
                {
                    return false;
                }

                visited.insert(current);

                for (const auto &next : graph.adjacencyList[current])
                {
                    path.push_back(next);
                    if (dfs(next, visited, path))
                    {
                        return true;
                    }
                    path.pop_back();
                }

                return false;
            };

            // Find path using DFS
            std::vector<std::string> pathIds = {startFix.getIdentifier()};
            std::unordered_set<std::string> visited;

            if (!dfs(startFix.getIdentifier(), visited, pathIds))
            {
                result.isValid = false;
                result.errors.push_back({INVALID_AIRWAY_DIRECTION,
                                         "Cannot traverse airway " + airway + " from " + startFix.getIdentifier() + " to " + endFix,
                                         0, "", ERROR});
                return result;
            }

            // Convert path to waypoints and validate levels
            std::vector<Waypoint> finalPath;
            uint32_t maxRequiredLevel = 0;
            std::optional<Waypoint> lastWaypoint = startFix;

            for (size_t i = 0; i < pathIds.size(); ++i)
            {
                auto waypoint = NavdataObject::FindClosestWaypointTo(pathIds[i], lastWaypoint);
                if (!waypoint)
                {
                    result.isValid = false;
                    result.errors.push_back({UNKNOWN_WAYPOINT, "Waypoint not found: " + pathIds[i], 0, "", ERROR});
                    return result;
                }

                finalPath.push_back(*waypoint);
                lastWaypoint = waypoint;

                // Check levels between consecutive waypoints
                if (i > 0)
                {
                    uint32_t level = graph.levels[pathIds[i - 1]][pathIds[i]];
                    maxRequiredLevel = std::max(maxRequiredLevel, level);
                }
            }

            // Validate flight level
            if (maxRequiredLevel > static_cast<uint32_t>(flightLevel))
            {
                result.isValid = false;
                result.errors.push_back({INSUFFICIENT_FLIGHT_LEVEL,
                                         "Required FL" + std::to_string(maxRequiredLevel),
                                         0, "", ERROR});
                return result;
            }

            // Build segment info
            for (size_t i = 0; i < finalPath.size() - 1; ++i)
            {
                AirwaySegmentInfo segment;
                segment.from = finalPath[i];
                segment.to = finalPath[i + 1];
                segment.minimum_level = graph.levels[finalPath[i].getIdentifier()][finalPath[i + 1].getIdentifier()];
                segment.canTraverse = true;
                result.segments.push_back(segment);
            }

            result.path = finalPath;
        }
        catch (const SQLite::Exception &e)
        {
            result.isValid = false;
            result.errors.push_back({INVALID_DATA, "Database error: " + std::string(e.what()), 0, "", ERROR});
        }

        return result;
    }

    bool AirwayNetwork::airwayExists(const std::string &airwayName)
    {
        try
        {
            SQLite::Statement stmt(db, "SELECT COUNT(*) FROM airways WHERE name = ?");
            stmt.bind(1, airwayName);

            if (stmt.executeStep())
            {
                int count = stmt.getColumn(0).getInt();
                return count > 0;
            }
        }
        catch (const SQLite::Exception &e)
        {
            std::cerr << "Database error: " << e.what() << std::endl;
        }
        return false;
    }

}
