#pragma once
#include "AirportConfigurator.h"
#include "Navdata.h"
#include "absl/strings/str_split.h"
#include "fmt/core.h"
#include "types/ParsedRoute.h"
#include "types/ParsingError.h"
#include "types/Procedure.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace RouteParser {

struct FoundProcedure {
    std::optional<std::string> procedure;
    std::optional<std::string> runway;
    std::optional<RouteParser::Procedure> extractedProcedure;
    std::vector<RouteParser::ParsingError> errors;
};

class SidStarParser {
public:
    static void AddSugggestedProcedures(ParsedRoute& parsedRoute,
        const std::string& origin, const std::string& destination,
        std::shared_ptr<AirportConfigurator> airportConfigurator)
    {
        if (!airportConfigurator) {
            return;
        }

        // Extract waypoint identifiers once
        std::vector<std::string> waypointIds;
        waypointIds.reserve(parsedRoute.waypoints.size());
        for (const auto& wpt : parsedRoute.waypoints) {
            waypointIds.push_back(wpt.getIdentifier());
        }

        // Handle SID suggestion
        if (parsedRoute.departureRunway.has_value() && !waypointIds.empty()) {
            // We have a departure runway but no SID - find a SID that matches both the
            // runway and first waypoint
            const std::string& runway = parsedRoute.departureRunway.value();
            const std::string& firstWaypoint = waypointIds[0];

            // Get all procedures for this airport
            auto procedures = NavdataObject::GetProcedures();

            // Find SIDs for this airport that match both the runway and first waypoint
            for (auto it = procedures.begin(); it != procedures.end(); ++it) {
                if (it->second.icao == origin && it->second.type == PROCEDURE_SID
                    && it->second.runway == runway) {

                    // Check if any of this procedure's waypoints match the first route
                    // waypoint
                    for (const auto& procWpt : it->second.waypoints) {
                        if (procWpt.getIdentifier() == firstWaypoint) {
                            // Found a procedure that matches both the runway and the
                            // first waypoint
                            parsedRoute.suggestedDepartureRunway = runway;
                            parsedRoute.suggestedSID = it->second;

                            // Add info-level parsing error about the suggestion
                            parsedRoute.errors.push_back(
                                { ParsingErrorType::NO_PROCEDURE_FOUND,
                                    "Suggesting SID " + it->second.name + " for runway "
                                        + runway,
                                    0, // Index 0 (start of route)
                                    origin, ParsingErrorLevel::INFO });

                            break;
                        }
                    }

                    if (parsedRoute.suggestedSID.has_value()) {
                        break; // Found a match, stop searching
                    }
                }
            }

        } else if (!parsedRoute.departureRunway.has_value()) {

            auto sidSuggestion = airportConfigurator->FindBestSID(origin, waypointIds);
            if (sidSuggestion) {
                parsedRoute.suggestedDepartureRunway = sidSuggestion->first;
                std::optional<Procedure> procedure = sidSuggestion->second;

                if (procedure.has_value()) {
                    parsedRoute.suggestedSID = *procedure;
                    parsedRoute.errors.push_back({ ParsingErrorType::NO_PROCEDURE_FOUND,
                        "Suggesting SID " + procedure->name + " for runway "
                            + sidSuggestion->first,
                        parsedRoute.totalTokens - 1, // Index of last token
                        destination, ParsingErrorLevel::INFO });
                }
            }
        }

        // Handle STAR suggestion
        if (parsedRoute.arrivalRunway.has_value() && !waypointIds.empty()) {
            // We have an arrival runway but no STAR - find a STAR that matches both
            // the runway and last waypoint
            const std::string& runway = parsedRoute.arrivalRunway.value();
            const std::string& lastWaypoint = waypointIds.back();

            // Get all procedures for this airport
            auto procedures = NavdataObject::GetProcedures();

            // Find STARs for this airport that match both the runway and last
            // waypoint
            for (auto it = procedures.begin(); it != procedures.end(); ++it) {
                if (it->second.icao == destination && it->second.type == PROCEDURE_STAR
                    && it->second.runway == runway) {

                    // Check if any of this procedure's waypoints match the last route
                    // waypoint
                    for (const auto& procWpt : it->second.waypoints) {
                        if (procWpt.getIdentifier() == lastWaypoint) {
                            // Found a procedure that matches both the runway and the
                            // last waypoint
                            parsedRoute.suggestedArrivalRunway = runway;
                            parsedRoute.suggestedSTAR = it->second;

                            // Add info-level parsing error about the suggestion
                            parsedRoute.errors.push_back(
                                { ParsingErrorType::NO_PROCEDURE_FOUND,
                                    "Suggesting STAR " + it->second.name + " for runway "
                                        + runway,
                                    parsedRoute.totalTokens - 1, // Index of last token
                                    destination, ParsingErrorLevel::INFO });

                            break;
                        }
                    }

                    if (parsedRoute.suggestedSTAR.has_value()) {
                        break; // Found a match, stop searching
                    }
                }
            }

            // If no match found, we don't suggest anything
        } else if (!parsedRoute.arrivalRunway.has_value()) {
            // Original behavior: No arrival runway specified
            auto starSuggestion
                = airportConfigurator->FindBestSTAR(destination, waypointIds);
            if (starSuggestion) {
                parsedRoute.suggestedArrivalRunway = starSuggestion->first;
                std::optional<Procedure> procedure = starSuggestion->second;

                if (procedure.has_value()) {
                    parsedRoute.suggestedSTAR = *procedure;
                    parsedRoute.errors.push_back({ ParsingErrorType::NO_PROCEDURE_FOUND,
                        "Suggesting STAR " + procedure->name + " for runway "
                            + starSuggestion->first,
                        parsedRoute.totalTokens - 1, // Index of last token
                        destination, ParsingErrorLevel::INFO });
                }
            }
        }
    }

    static std::optional<std::string> FindRunway(const std::string& token)
    {
        auto slashPos = token.find('/');
        if (slashPos != std::string::npos) {
            const std::vector<std::string> parts = absl::StrSplit(token, '/');
            if (parts.size() > 1 && (parts[1].size() == 2 || parts[1].size() == 3)) {
                return parts[1]; // Found a runway
            }
        }
        return std::nullopt;
    }

    static FoundProcedure FindProcedure(const std::string& token,
        const std::string& anchorIcao, ProcedureType type, int tokenIndex)
    {
        if (token.empty() || anchorIcao.empty()) {
            return FoundProcedure { {}, {}, {},
                { ParsingError { ParsingErrorType::INVALID_DATA, "Empty token or ICAO",
                    tokenIndex, token } } };
        }

        // Extract runway if present
        std::optional<std::string> runway = FindRunway(token);
        const std::vector<std::string> parts = absl::StrSplit(token, '/');
        std::string procedureToken = parts[0];

        // Check if this is a procedure pattern (2-5 letters followed by 1-2 digits
        // and optional letter)
        bool isProcedurePattern = std::regex_match(
            procedureToken, std::regex(R"([A-Z]{3,5}\d[A-Z]?(?:.*)?(?:\/\d{2}[LRC]?)?)"));

        // Check if this is an airport code pattern (exactly 4 letters)
        bool isAirportPattern = (procedureToken.length() == 4
            && std::all_of(procedureToken.begin(), procedureToken.end(),
                [](char c) { return std::isalpha(c); }));

        auto to_upper = [](const std::string& s) {
            std::string upper(s);
            std::transform(upper.begin(), upper.end(), upper.begin(),
                [](unsigned char c) { return std::toupper(c); });
            return upper;
        };

        // Prioritize procedure lookup if it matches the pattern
        std::vector<RouteParser::Procedure> matchingProcedures;
        if (isProcedurePattern) {
            const auto& procedures = NavdataObject::GetProcedures();
            auto range = procedures.equal_range(procedureToken);

            std::vector<RouteParser::Procedure> matchingProcedures;
            for (const auto& [key, proc] : procedures) {

                if (to_upper(key) == to_upper(procedureToken) && proc.icao == anchorIcao
                    && proc.type == type) {
                    matchingProcedures.push_back(proc);
                }
            }

            // If we have procedures and a runway, try to find an exact match
            if (!matchingProcedures.empty() && runway) {
                for (const auto& procedure : matchingProcedures) {
                    if (procedure.runway == runway.value()) {
                        // Full valid match for procedure + runway, highest priority
                        return FoundProcedure { procedureToken, runway, procedure };
                    }
                }
                std::string typeStr = (type == PROCEDURE_SID) ? "SID" : "STAR";
                // No exact runway match but we have procedures
                return FoundProcedure { std::nullopt, std::nullopt, std::nullopt,

                    { ParsingError { ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
                        fmt::format("No matching runway {} found for procedure {} at {}, "
                                    "ignoring confirmed {}",
                            runway.value_or("N/A"), procedureToken, anchorIcao, typeStr),
                        tokenIndex, procedureToken, ParsingErrorLevel::PARSE_ERROR } } };
            }
        }

        // Handle airport code + runway case (lower priority than procedures)
        if (isAirportPattern && runway) {
            // Validate the runway exists at the airport
            if (procedureToken != anchorIcao) {
                // Airport doesn't match - generate INFO level error
                return FoundProcedure { std::nullopt, std::nullopt, std::nullopt,
                    { ParsingError { ParsingErrorType::PROCEDURE_AIRPORT_MISMATCH,
                        fmt::format("Airport code {} doesn't match expected {}",
                            procedureToken, anchorIcao),
                        tokenIndex, token, ParsingErrorLevel::INFO } } };
            }
            auto runwayNetwork = NavdataObject::GetRunwayNetwork();
            if (runwayNetwork) {
                bool runwayExists = runwayNetwork->runwayExistsAtAirport(
                    procedureToken, runway.value());

                if (!runwayExists) {
                    return FoundProcedure { std::nullopt, std::nullopt, std::nullopt,
                        { ParsingError { ParsingErrorType::INVALID_RUNWAY,
                            fmt::format("Runway {} not found at airport {}",
                                runway.value_or("N/A"), procedureToken),
                            tokenIndex, token, ParsingErrorLevel::PARSE_ERROR } } };
                }
            }

            // Valid airport + runway combination
            return FoundProcedure { std::nullopt, runway, std::nullopt };
        }

        // Nothing found
        return FoundProcedure { std::nullopt, std::nullopt, std::nullopt };
    }
};
}; // namespace RouteParser