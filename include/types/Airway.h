#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdexcept>
#include <erkir/geo/sphericalpoint.h>
#include <memory>
#include <algorithm>
#include "exceptions/AirwayExceptions.h"

namespace RouteParser
{

    enum class AirwayLevel
    {
        BOTH, // 'B' - Both high and low
        HIGH, // 'H' - High level only
        LOW,  // 'L' - Low level only
        UNKNOWN
    };

    constexpr AirwayLevel stringToAirwayLevel(const char *level)
    {
        return (level[0] == 'B') ? AirwayLevel::BOTH : (level[0] == 'H') ? AirwayLevel::HIGH
                                                   : (level[0] == 'L')   ? AirwayLevel::LOW
                                                                         : AirwayLevel::UNKNOWN;
    }

    struct AirwayFix
    {
        std::string name;
        erkir::spherical::Point coord;
        std::optional<uint32_t> minimum_level;

        struct Hash
        {
            size_t operator()(const AirwayFix &fix) const
            {
                return std::hash<std::string>{}(fix.name);
            }
        };
    };

    class Airway
    {
    private:
        struct AirwaySegment
        {
            size_t from_idx;
            size_t to_idx;
            uint32_t minimum_level;
            bool canTraverse;
        };

        std::vector<AirwayFix> fixes;
        std::unordered_map<std::string, size_t> fix_indices;
        std::vector<AirwaySegment> segments;

        std::vector<std::vector<size_t>> adjacency_list;

        size_t getFixIndex(const AirwayFix &fix)
        {
            auto it = fix_indices.find(fix.name);
            if (it != fix_indices.end())
            {
                return it->second;
            }
            size_t idx = fixes.size();
            fixes.push_back(fix);
            fix_indices[fix.name] = idx;
            adjacency_list.push_back({});
            return idx;
        }

    public:
        std::string name;
        AirwayLevel level;

        bool addSegment(const AirwayFix &from, const AirwayFix &to,
                        uint32_t minLevel, bool canTraverse)
        {
            size_t from_idx = getFixIndex(from);
            size_t to_idx = getFixIndex(to);

            // Check for existing segment
            for (size_t i = 0; i < segments.size(); ++i)
            {
                if (segments[i].from_idx == from_idx && segments[i].to_idx == to_idx)
                {
                    segments[i].canTraverse = canTraverse;
                    return true;
                }
            }

            // Add new segment
            segments.push_back({from_idx, to_idx, minLevel, canTraverse});

            // Update adjacency list with segment index
            if (canTraverse)
            {
                adjacency_list[from_idx].push_back(segments.size() - 1);
            }

            return true;
        }

        std::vector<AirwayFix> getFixesBetween(const std::string &from,
                                               const std::string &to) const
        {
            if (from == to)
            {
                throw AirwayRoutingException(
                    "Entry and exit fixes must be different for airway " + name);
            }

            auto from_it = fix_indices.find(from);
            auto to_it = fix_indices.find(to);

            if (from_it == fix_indices.end() || to_it == fix_indices.end())
            {
                throw FixNotFoundException("Fix not found");
            }

            auto path = findPathBFS(from_it->second, to_it->second);
            if (path.empty())
            {
                throw InvalidAirwayDirectionException(
                    "Cannot traverse airway " + name + " from " + from + " to " + to);
            }
            return path;
        }

        bool hasFix(const std::string &fixName) const
        {
            return fix_indices.find(fixName) != fix_indices.end();
        }

    private:
        std::vector<AirwayFix> findPathBFS(size_t start_idx, size_t target_idx) const
        {
            std::vector<bool> visited(fixes.size(), false);
            std::vector<size_t> parent(fixes.size(), -1);
            std::vector<size_t> parent_segment(fixes.size(), -1);
            std::vector<size_t> queue;

            queue.push_back(start_idx);
            visited[start_idx] = true;

            bool found = false;
            while (!queue.empty() && !found)
            {
                size_t current = queue.front();
                queue.erase(queue.begin());

                // Check each outgoing segment from current fix
                for (size_t seg_idx : adjacency_list[current])
                {
                    const auto &segment = segments[seg_idx];

                    // Only follow traversable segments
                    if (!segment.canTraverse)
                    {
                        continue;
                    }

                    size_t next_idx = segment.to_idx;
                    if (!visited[next_idx])
                    {
                        visited[next_idx] = true;
                        parent[next_idx] = current;
                        parent_segment[next_idx] = seg_idx;
                        queue.push_back(next_idx);

                        if (next_idx == target_idx)
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (!found)
            {
                return {}; // Return empty path if no valid route found
            }

            // Reconstruct path
            std::vector<AirwayFix> path;
            for (size_t current = target_idx; current != -1; current = parent[current])
            {
                path.push_back(fixes[current]);
            }
            std::reverse(path.begin(), path.end());

            return path;
        }
    };

    class AirwayNetwork
    {
    private:
        std::unordered_map<std::string, Airway> airways;

    public:
        bool addAirwaySegment(const std::string &airwayName,
                              const std::string &levelChar,
                              const AirwayFix &from,
                              const AirwayFix &to,
                              uint32_t minLevel,
                              bool canTraverse)
        {
            auto &airway = airways[airwayName];
            if (airway.name.empty())
            {
                airway.name = airwayName;
                airway.level = stringToAirwayLevel(levelChar.c_str());
            }
            return airway.addSegment(from, to, minLevel, canTraverse);
        }

        const Airway *getAirway(const std::string &name) const
        {
            auto it = airways.find(name);
            if (it == airways.end())
            {
                throw AirwayNotFoundException("Airway not found: " + name);
            }
            return &it->second;
        }

        std::vector<std::string> getAllAirways() const
        {
            std::vector<std::string> names;
            names.reserve(airways.size());
            for (const auto &pair : airways)
            {
                names.push_back(pair.first);
            }
            return names;
        }
    };

} // namespace RouteParser