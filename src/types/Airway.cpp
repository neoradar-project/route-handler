#include "types/Airway.h"
#include <set>
#include <functional>
namespace RouteParser
{
    std::string Airway::makePathKey(const std::string &from, const std::string &to) const
    {
        return from + ":" + to;
    }

    size_t Airway::addFix(const AirwayFix &fix)
    {
        // Reserve space initially to avoid reallocations
        if (fixes.empty())
        {
            fixes.reserve(32); // Typical airway size
            fix_indices.reserve(32);
        }

        auto it = fix_indices.find(fix.name);
        if (it != fix_indices.end())
        {
            return it->second; // Skip distance check since name matches are unique
        }

        size_t idx = fixes.size();
        fixes.push_back(fix);
        fix_indices.emplace(fix.name, idx); // Use emplace instead of operator[]
        return idx;
    }

    void Airway::setLevel(const std::string &levelChar)
    {
        level = stringToAirwayLevel(levelChar.c_str());
    }

    bool Airway::addSegment(const AirwayFix &from, const AirwayFix &to,
                            uint32_t minLevel, bool canTraverse)
    {
        if (connections.empty())
        {
            connections.reserve(32); // Pre-allocate space
        }

        size_t fromIdx = addFix(from);
        size_t toIdx = addFix(to);

        // Store connection indices for quick lookup
        auto connKey = std::make_pair(fromIdx, toIdx);
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

    void Airway::computeFixOrder()
    {
        if (fixes.empty())
            return;

        // Count incoming and outgoing edges
        std::vector<int> inDegree(fixes.size(), 0);
        std::vector<int> outDegree(fixes.size(), 0);

        for (const auto &conn : connections)
        {
            if (conn.canTraverse)
            {
                inDegree[conn.to_idx]++;
                outDegree[conn.from_idx]++;
            }
        }

        // Find the start point (node with no incoming edges or lowest inDegree)
        size_t startIdx = 0;
        int minInDegree = std::numeric_limits<int>::max();
        for (size_t i = 0; i < fixes.size(); i++)
        {
            if (inDegree[i] < minInDegree && outDegree[i] > 0)
            {
                minInDegree = inDegree[i];
                startIdx = i;
            }
        }

        // Build ordered list using DFS
        ordered_fixes.clear();
        ordered_fixes.reserve(fixes.size());
        std::vector<bool> visited(fixes.size(), false);

        std::function<void(size_t)> dfs = [&](size_t current)
        {
            visited[current] = true;
            ordered_fixes.push_back(fixes[current]);

            // Find next traversable connection
            for (const auto &conn : connections)
            {
                if (conn.from_idx == current && conn.canTraverse && !visited[conn.to_idx])
                {
                    dfs(conn.to_idx);
                }
            }
        };

        dfs(startIdx);

        // Add any remaining unvisited fixes
        for (size_t i = 0; i < fixes.size(); i++)
        {
            if (!visited[i])
            {
                ordered_fixes.push_back(fixes[i]);
            }
        }

        // Update the fixes vector to match the ordered fixes
        fixes = ordered_fixes;
    }

    void Airway::precomputePaths()
    {
        path_cache.clear();

        // Pre-compute paths between all pairs of fixes
        for (const auto &from : fixes)
        {
            for (const auto &to : fixes)
            {
                if (from == to)
                    continue;

                try
                {
                    auto path = findPath(from.name, to.name);
                    if (!path.empty())
                    {
                        path_cache[makePathKey(from.name, to.name)] = path;
                    }
                }
                catch (const InvalidAirwayDirectionException &)
                {
                    continue;
                }
            }
        }
    }

    void Airway::finalizeAirway()
    {
        computeFixOrder();
    }

    std::vector<AirwaySegmentInfo> Airway::findPath(const std::string &from,
                                                    const std::string &to) const
    {
        auto fromIt = fix_indices.find(from);
        auto toIt = fix_indices.find(to);

        if (fromIt == fix_indices.end() || toIt == fix_indices.end())
        {
            throw FixNotFoundException("Fix not found in airway " + name);
        }

        std::vector<int> parent(fixes.size(), -1);
        std::vector<int> connection_idx(fixes.size(), -1);
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

    // std::vector<AirwaySegmentInfo> Airway::getSegmentsBetween(
    //     const std::string &from, const std::string &to) const
    // {
    //     // Check pre-computed cache first
    //     auto cacheKey = makePathKey(from, to);
    //     auto it = path_cache.find(cacheKey);
    //     if (it != path_cache.end())
    //     {
    //         return it->second;
    //     }

    //     // Fallback to computing path if not found (shouldn't happen after initialization)
    //     return findPath(from, to);
    // }

    const std::vector<AirwayFix> &Airway::getAllFixes() const
    {
        return fixes;
    }

    const std::vector<AirwayFix> &Airway::getFixesInOrder() const
    {
        return ordered_fixes;
    }

    bool Airway::hasFix(const std::string &fixName) const
    {
        return fix_indices.find(fixName) != fix_indices.end();
    }

    bool AirwayNetwork::addAirwaySegment(const std::string &airwayName,
                                         const std::string &levelChar,
                                         const AirwayFix &from,
                                         const AirwayFix &to,
                                         uint32_t minLevel,
                                         bool canTraverse)
    {
        // Pre-allocate space for airways
        if (airways.empty())
        {
            airways.reserve(1000); // Typical number of airways
        }

        std::shared_ptr<Airway> targetAirway;
        auto range = airways.equal_range(airwayName);

        // Try to find matching airway
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second->hasFix(from.name) || it->second->hasFix(to.name))
            {
                targetAirway = it->second;
                break;
            }
        }

        if (!targetAirway)
        {
            targetAirway = std::make_shared<Airway>();
            targetAirway->name = airwayName;
            targetAirway->setLevel(levelChar);
            airways.emplace(airwayName, targetAirway);
        }

        return targetAirway->addSegment(from, to, minLevel, canTraverse);
    }

    std::vector<AirwaySegmentInfo> Airway::getSegmentsBetween(
        const std::string &from, const std::string &to) const
    {
        // Find indices in ordered fixes
        auto fromIt = fix_indices.find(from);
        auto toIt = fix_indices.find(to);

        if (fromIt == fix_indices.end() || toIt == fix_indices.end())
        {
            throw FixNotFoundException("Fix not found in airway " + name);
        }

        // Convert ordered fixes between from and to into segments
        std::vector<AirwaySegmentInfo> result;
        bool started = false;
        const AirwayFix *lastFix = nullptr;

        for (const auto &fix : ordered_fixes)
        {
            if (fix.name == from)
            {
                started = true;
                lastFix = &fix;
                continue;
            }

            if (started)
            {
                // Find the connection for these fixes
                size_t fromIdx = fix_indices.at(lastFix->name);
                size_t toIdx = fix_indices.at(fix.name);

                uint32_t minLevel = 0;
                bool canTraverse = false;

                // Find matching connection
                for (const auto &conn : connections)
                {
                    if (conn.from_idx == fromIdx && conn.to_idx == toIdx)
                    {
                        minLevel = conn.minimum_level;
                        canTraverse = conn.canTraverse;
                        break;
                    }
                }

                if (!canTraverse)
                {
                    throw InvalidAirwayDirectionException(
                        "Cannot traverse airway " + name + " from " + from + " to " + to);
                }

                result.push_back({*lastFix, fix, minLevel, canTraverse});
                lastFix = &fix;
            }

            if (fix.name == to)
            {
                break;
            }
        }

        if (result.empty())
        {
            throw InvalidAirwayDirectionException(
                "Cannot traverse airway " + name + " from " + from + " to " + to);
        }

        return result;
    }

    const Airway *AirwayNetwork::findNearestAirway(
        const std::string &name,
        const erkir::spherical::Point &point) const
    {
        auto range = airways.equal_range(name);
        const Airway *nearest = nullptr;
        double minDist = std::numeric_limits<double>::max();

        for (auto it = range.first; it != range.second; ++it)
        {
            for (const auto &fix : it->second->getAllFixes())
            {
                double dist = fix.coord.distanceTo(point);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = it->second.get();
                }
            }
        }

        return nearest;
    }

    std::vector<AirwayFix> AirwayNetwork::getFixesBetween(
        const std::string &airwayName,
        const std::string &from,
        const std::string &to,
        const erkir::spherical::Point &nearPoint) const
    {
        auto segments = getSegmentsBetween(airwayName, from, to, nearPoint);

        std::vector<AirwayFix> result;
        if (!segments.empty())
        {
            result.reserve(segments.size() + 1);
            result.push_back(segments.front().from); // Add first fix
            for (const auto &segment : segments)
            {
                result.push_back(segment.to); // Add all destination fixes
            }
        }

        return result;
    }

    std::vector<AirwaySegmentInfo> AirwayNetwork::getSegmentsBetween(
        const std::string &airwayName,
        const std::string &from,
        const std::string &to,
        const erkir::spherical::Point &nearPoint) const
    {
        const auto *airway = findNearestAirway(airwayName, nearPoint);
        if (!airway)
        {
            return {};
        }

        return airway->getSegmentsBetween(from, to);
    }

    const Airway *AirwayNetwork::getAirway(const std::string &name) const
    {
        auto it = airways.find(name);
        if (it == airways.end())
        {
            throw AirwayNotFoundException("Airway not found: " + name);
        }
        return it->second.get();
    }

    const Airway *AirwayNetwork::getAirwayByLatLon(
        const std::string &name,
        const erkir::spherical::Point &latLon) const
    {
        const auto *airway = findNearestAirway(name, latLon);
        if (!airway)
        {
            throw AirwayNotFoundException("Airway " + name + " not found");
        }
        return airway;
    }

    std::vector<std::string> AirwayNetwork::getAllAirways() const
    {
        std::vector<std::string> names;
        names.reserve(airways.size());

        for (const auto &[name, _] : airways)
        {
            if (std::find(names.begin(), names.end(), name) == names.end())
            {
                names.push_back(name);
            }
        }
        return names;
    }

    void AirwayNetwork::finalizeAirways()
    {
        for (auto &[name, airway] : airways)
        {
            try
            {
                airway->finalizeAirway();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Failed to finalize airway " << name
                          << ": " << e.what() << std::endl;
            }
        }
    }

} // namespace RouteParser