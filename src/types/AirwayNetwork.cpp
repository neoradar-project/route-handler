#include "types/AirwayNetwork.h"
#include <algorithm>
#include <limits>
#include <sstream>

namespace RouteParser
{
    std::shared_ptr<const Airway> AirwayNetwork::findNearestAirway(const std::string &name, const erkir::spherical::Point &point) const
    {
        auto range = airways.equal_range(name);
        std::shared_ptr<const Airway> nearest = nullptr;
        double minDist = std::numeric_limits<double>::max();

        for (auto it = range.first; it != range.second; ++it)
        {
            for (const auto &fix : it->second->getAllFixes())
            {
                double dist = fix.getPosition().distanceTo(point);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearest = it->second;
                }
            }
        }

        return nearest;
    }

    RouteValidationResult AirwayNetwork::validateRoute(const std::string &routeString) const
    {
        std::vector<RouteSegment> segments;
        std::istringstream iss(routeString);
        std::string token;
        std::string prevFix;
        std::string currentAirway;

        while (std::getline(iss >> std::ws, token, ' '))
        {
            if (prevFix.empty())
            {
                prevFix = token;
                continue;
            }

            if (currentAirway.empty())
            {
                currentAirway = token;
                continue;
            }

            segments.push_back({prevFix, currentAirway, token});
            prevFix = token;
            currentAirway.clear();
        }

        if (!currentAirway.empty())
        {
            return {false, {ParsingError{INVALID_AIRWAY_FORMAT, "Route string ends with airway identifier: " + currentAirway, 0, "", ERROR}}, {}};
        }
        erkir::spherical::Point startPoint;
        bool foundStart = false;

        for (const auto &[name, airway] : airways)
        {
            if (airway->hasFix(segments.front().from))
            {
                const auto &fixes = airway->getAllFixes();
                auto it = std::find_if(fixes.begin(), fixes.end(),
                                       [&](const Waypoint &fix)
                                       { return fix.getIdentifier() == segments.front().from; });
                if (it != fixes.end())
                {
                    startPoint = it->getPosition();
                    foundStart = true;
                    break;
                }
            }
        }

        if (!foundStart)
        {
            return {false, {ParsingError{AIRWAY_FIX_NOT_FOUND, "Could not find starting fix: " + segments.front().from, 0, "", ERROR}}, {}};
        }

        return validateSegments(segments, startPoint);
    }

    RouteValidationResult AirwayNetwork::validateSegments(const std::vector<RouteSegment> &segments,
                                                          const erkir::spherical::Point &startPoint) const
    {
        std::vector<AirwaySegmentInfo> validatedSegments;
        erkir::spherical::Point currentPoint = startPoint;

        for (const auto &segment : segments)
        {
            try
            {
                const auto *airway = findNearestAirway(segment.airwayName, currentPoint).get();
                if (!airway)
                {
                    return {false, {ParsingError{UNKNOWN_AIRWAY, "Could not find airway: " + segment.airwayName, 0, "", ERROR}}, {}};
                }

                auto pathSegments = airway->getSegmentsBetween(segment.from, segment.to);

                if (!pathSegments.empty())
                {
                    currentPoint = pathSegments.back().to.getPosition();
                    validatedSegments.insert(validatedSegments.end(),
                                             pathSegments.begin(),
                                             pathSegments.end());
                }
            }
            // In validateSegments function
            catch (const InvalidAirwayDirectionException &e)
            {
                return {false,
                        {ParsingError{INVALID_AIRWAY_DIRECTION, e.what(),
                                      0, "", ERROR}},
                        {}};
            }
            catch (const FixNotFoundException &e)
            {
                return {false,
                        {ParsingError{AIRWAY_FIX_NOT_FOUND, e.what(), 0, "", ERROR}},
                        {}};
            }
            catch (const std::exception &e)
            {
                return {false,
                        {ParsingError{INVALID_DATA, "Error validating route: " + std::string(e.what()), 0, "", ERROR}},
                        {}};
            }
        }

        return {
            true,
            {},
            validatedSegments};
    }

    std::vector<std::shared_ptr<const Airway>> AirwayNetwork::getAirwaysByNameAndFix(const std::string &name,
                                                                                     const std::string &fixName) const
    {
        std::vector<std::shared_ptr<const Airway>> result;
        auto range = airways.equal_range(name);

        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second->hasFix(fixName))
            {
                result.push_back(it->second);
            }
        }

        return result;
    }

    bool AirwayNetwork::addAirwaySegment(const std::string &airwayName,
                                         const std::string &levelChar,
                                         const Waypoint &from,
                                         const Waypoint &to,
                                         uint32_t minLevel,
                                         bool canTraverse)
    {
        std::shared_ptr<Airway> targetAirway;
        auto range = airways.equal_range(airwayName);

        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second->level == stringToAirwayLevel(levelChar.c_str()))
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

    std::vector<AirwaySegmentInfo> AirwayNetwork::getSegmentsBetween(const std::string &airwayName,
                                                                     const std::string &from,
                                                                     const std::string &to,
                                                                     const erkir::spherical::Point &nearPoint) const
    {
        const auto *airway = findNearestAirway(airwayName, nearPoint).get();
        if (!airway)
        {
            return {};
        }

        return airway->getSegmentsBetween(from, to);
    }

    std::vector<Waypoint> AirwayNetwork::getFixesBetween(const std::string &airwayName,
                                                         const std::string &from,
                                                         const std::string &to,
                                                         const erkir::spherical::Point &nearPoint) const
    {
        auto segments = getSegmentsBetween(airwayName, from, to, nearPoint);

        std::vector<Waypoint> result;
        if (!segments.empty())
        {
            result.reserve(segments.size() + 1);
            result.push_back(segments.front().from);
            for (const auto &segment : segments)
            {
                result.push_back(segment.to);
            }
        }

        return result;
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

    std::shared_ptr<const Airway> AirwayNetwork::getAirway(const std::string &name) const
    {
        auto it = airways.find(name);
        if (it == airways.end())
        {
            throw AirwayNotFoundException("Airway not found: " + name);
        }
        return it->second;
    }

    std::shared_ptr<const Airway> AirwayNetwork::getAirwayByLatLon(const std::string &name,
                                                                   const erkir::spherical::Point &latLon) const
    {
        auto airway = findNearestAirway(name, latLon);
        if (!airway)
        {
            throw AirwayNotFoundException("Airway " + name + " not found");
        }
        return airway;
    }

    void AirwayNetwork::finalizeAirways()
    {
        for (auto &[name, airway] : airways)
        {
            try
            {
                airway->finalizeAirway();
            }
            catch (...)
            {
                // Failed to finalize airway
            }
        }
    }

    RouteValidationResult AirwayNetwork::validateAirwayTraversal(
        const std::string &startFix,
        const std::string &airwayName,
        const std::string &endFix,
        uint32_t flightLevel,
        const erkir::spherical::Point &nearPoint) const
    {
        auto range = airways.equal_range(airwayName);

        if (range.first == range.second)
        {
            return {false,
                    {ParsingError{UNKNOWN_AIRWAY, "Airway not found: " + airwayName, 0, "", ERROR}},
                    {}};
        }

        // Collect airways that contain both fixes and calculate their distances
        std::vector<std::pair<std::shared_ptr<const Airway>, double>> validAirways;
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second->hasFix(startFix) && it->second->hasFix(endFix))
            {
                double minDist = std::numeric_limits<double>::max();
                for (const auto &fix : it->second->getAllFixes())
                {
                    double dist = fix.getPosition().distanceTo(nearPoint);
                    minDist = std::min(minDist, dist);
                }
                validAirways.push_back({it->second, minDist});
            }
        }

        if (validAirways.empty())
        {
            return {false,
                    {ParsingError{AIRWAY_FIX_NOT_FOUND, "No valid airway found containing both fixes", 0, "", ERROR}},
                    {}};
        }

        // Sort airways by distance
        std::sort(validAirways.begin(), validAirways.end(),
                  [](const auto &a, const auto &b)
                  { return a.second < b.second; });

        // Group sorted airways by level type
        std::map<AirwayLevel, std::vector<std::shared_ptr<const Airway>>> levelAirways;
        for (const auto &[airway, _] : validAirways)
        {
            levelAirways[airway->level].push_back(airway);
        }

        std::vector<ParsingError> errors;
        uint32_t lowestRequired = std::numeric_limits<uint32_t>::max();

        // Try each airway level
        for (const auto &[level, airways] : levelAirways)
        {
            for (const auto &airway : airways)
            {
                try
                {
                    auto segments = airway->getSegmentsBetween(startFix, endFix);
                    if (segments.empty())
                        continue;

                    // Check if all segments meet minimum level requirements
                    bool canTraverseAtLevel = true;
                    uint32_t maxRequiredLevel = 0;

                    for (const auto &segment : segments)
                    {
                        if (segment.minimum_level > flightLevel)
                        {
                            canTraverseAtLevel = false;
                            maxRequiredLevel = std::max(maxRequiredLevel, segment.minimum_level);
                        }
                    }

                    if (canTraverseAtLevel)
                    {
                        return {true, {}, segments};
                    }

                    lowestRequired = std::min(lowestRequired, maxRequiredLevel);
                    errors.push_back({INSUFFICIENT_FLIGHT_LEVEL,
                                      std::string(airwayLevelToString(level)) + " airway requires FL" + std::to_string(maxRequiredLevel),
                                      0, "", INFO});
                }
                catch (const InvalidAirwayDirectionException &)
                {
                    errors.push_back({INVALID_AIRWAY_DIRECTION,
                                      std::string("Cannot traverse ") + airwayLevelToString(level) + " airway in this direction",
                                      0, "", INFO});
                }
            }
        }

        if (errors.empty())
        {
            errors.push_back({INVALID_AIRWAY_DIRECTION,
                              "No valid route found between fixes on " + airwayName,
                              0, "", ERROR});
        }

        return {false, errors, {}};
    }
} // namespace RouteParser