#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include "AirwayTypes.h"
#include "exceptions/AirwayExceptions.h"
#include <functional>
#include "types/Waypoint.h"
namespace RouteParser
{

    class Airway
    {
    private:
        struct Connection
        {
            size_t from_idx;
            size_t to_idx;
            uint32_t minimum_level;
            bool canTraverse;
        };

        std::vector<Waypoint> fixes;
        std::vector<Waypoint> ordered_fixes;
        std::vector<Connection> connections;
        std::unordered_map<std::string, size_t> fix_indices;
        std::unordered_map<std::string, std::vector<AirwaySegmentInfo>> path_cache;

        inline size_t addFix(const Waypoint &fix)
        {
            if (fixes.empty())
            {
                fixes.reserve(32);
                fix_indices.reserve(32);
            }

            auto it = fix_indices.find(fix.getIdentifier());
            if (it != fix_indices.end())
            {
                return it->second;
            }

            size_t idx = fixes.size();
            fixes.push_back(fix);
            fix_indices.emplace(fix.getIdentifier(), idx);
            return idx;
        }

        inline std::string makePathKey(const std::string &from, const std::string &to) const
        {
            return from + ":" + to;
        }

        void computeFixOrder()
        {
            if (fixes.empty())
                return;

            bool isDirectional = false;
            for (const auto &conn : connections)
            {
                auto reverseConn = std::find_if(connections.begin(), connections.end(),
                                                [&](const Connection &c)
                                                {
                                                    return c.from_idx == conn.to_idx && c.to_idx == conn.from_idx;
                                                });

                if (reverseConn == connections.end() || !reverseConn->canTraverse)
                {
                    isDirectional = true;
                    break;
                }
            }

            if (isDirectional)
            {
                computeDirectionalOrder();
            }
            else
            {
                computeGeographicOrder();
            }
        }

        void computeDirectionalOrder()
        {
            std::vector<std::vector<size_t>> adj(fixes.size());
            std::vector<int> inDegree(fixes.size(), 0);

            for (const auto &conn : connections)
            {
                if (conn.canTraverse)
                {
                    adj[conn.from_idx].push_back(conn.to_idx);
                    inDegree[conn.to_idx]++;
                }
            }

            size_t startIdx = 0;
            for (size_t i = 0; i < fixes.size(); i++)
            {
                if (inDegree[i] == 0 && !adj[i].empty())
                {
                    startIdx = i;
                    break;
                }
            }

            std::vector<bool> visited(fixes.size(), false);
            ordered_fixes.clear();
            ordered_fixes.reserve(fixes.size());

            std::function<void(size_t)> dfs = [&](size_t current)
            {
                visited[current] = true;
                ordered_fixes.push_back(fixes[current]);

                for (size_t next : adj[current])
                {
                    if (!visited[next])
                    {
                        dfs(next);
                    }
                }
            };

            dfs(startIdx);

            for (size_t i = 0; i < fixes.size(); i++)
            {
                if (!visited[i])
                {
                    ordered_fixes.push_back(fixes[i]);
                }
            }
        }

        void computeGeographicOrder()
        {
            ordered_fixes = fixes;
            std::sort(ordered_fixes.begin(), ordered_fixes.end(),
                      [](const Waypoint &a, const Waypoint &b)
                      {
                          return a.getPosition().longitude().degrees() < b.getPosition().longitude().degrees();
                      });
        }

        std::vector<AirwaySegmentInfo> findPath(const std::string &from, const std::string &to) const
        {
            auto fromIt = fix_indices.find(from);
            auto toIt = fix_indices.find(to);

            if (fromIt == fix_indices.end() || toIt == fix_indices.end())
            {
                throw FixNotFoundException("Fix not found in airway " + name);
            }

            std::vector<size_t> parent(fixes.size(), static_cast<size_t>(-1));
            std::vector<size_t> connection_idx(fixes.size(), static_cast<size_t>(-1));
            std::vector<bool> visited(fixes.size(), false);
            std::queue<size_t> queue;

            queue.push(fromIt->second);
            visited[fromIt->second] = true;

            bool found = false;
            while (!queue.empty() && !found)
            {
                size_t current = queue.front();
                queue.pop();

                for (size_t i = 0; i < connections.size(); ++i)
                {
                    const auto &conn = connections[i];
                    if (conn.from_idx != current || !conn.canTraverse)
                    {
                        continue;
                    }

                    if (!visited[conn.to_idx])
                    {
                        visited[conn.to_idx] = true;
                        parent[conn.to_idx] = current;
                        connection_idx[conn.to_idx] = i;
                        queue.push(conn.to_idx);

                        if (conn.to_idx == toIt->second)
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (!found)
            {
                throw InvalidAirwayDirectionException(
                    "Cannot traverse airway " + name + " from " + from + " to " + to);
            }

            std::vector<AirwaySegmentInfo> path;
            for (size_t current = toIt->second; current != fromIt->second;
                 current = parent[current])
            {
                const auto &conn = connections[connection_idx[current]];
                path.push_back({fixes[conn.from_idx],
                                fixes[conn.to_idx],
                                conn.minimum_level,
                                conn.canTraverse});
            }
            std::reverse(path.begin(), path.end());

            return path;
        }

    public:
        std::string name;
        AirwayLevel level{AirwayLevel::UNKNOWN};

        void setLevel(const std::string &levelChar)
        {
            level = stringToAirwayLevel(levelChar.c_str());
        }

        bool addSegment(const Waypoint &from, const Waypoint &to,
                        uint32_t minLevel, bool canTraverse)
        {
            if (connections.empty())
            {
                connections.reserve(32);
            }

            size_t fromIdx = addFix(from);
            size_t toIdx = addFix(to);

            auto it = std::find_if(connections.begin(), connections.end(),
                                   [fromIdx, toIdx](const Connection &conn)
                                   {
                                       return conn.from_idx == fromIdx && conn.to_idx == toIdx;
                                   });

            if (it != connections.end())
            {
                it->canTraverse = canTraverse;
                it->minimum_level = minLevel;
                return true;
            }

            connections.push_back({fromIdx, toIdx, minLevel, canTraverse});
            return true;
        }

        void finalizeAirway()
        {
            computeFixOrder();
        }

        std::vector<AirwaySegmentInfo> getSegmentsBetween(
            const std::string &from, const std::string &to) const
        {
            return findPath(from, to);
        }

        const std::vector<Waypoint> &getAllFixes() const
        {
            return fixes;
        }

        const std::vector<Waypoint> &getFixesInOrder() const
        {
            return ordered_fixes;
        }

        bool hasFix(const std::string &fixName) const
        {
            return fix_indices.find(fixName) != fix_indices.end();
        }
    };

} // namespace RouteParser