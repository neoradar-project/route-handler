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

        std::vector<std::string> waypointIds;
        waypointIds.reserve(parsedRoute.waypoints.size());
        for (const auto& wpt : parsedRoute.waypoints) {
            waypointIds.push_back(wpt.getIdentifier());
        }

        // Handle SID suggestion
        if (parsedRoute.departureRunway.has_value() && !waypointIds.empty()) {
            const std::string& runway = parsedRoute.departureRunway.value();
            const std::string& firstWaypoint = waypointIds[0];

            auto airportProcedures = NavdataObject::GetProceduresByAirport(origin);
            const auto& procedures = NavdataObject::GetProcedures();

            for (size_t idx : airportProcedures) {
                const auto& procedure = procedures[idx];
                if (procedure.type == PROCEDURE_SID && procedure.runway == runway) {
                    for (const auto& procWpt : procedure.waypoints) {
                        if (procWpt.getIdentifier() == firstWaypoint) {
                            parsedRoute.suggestedDepartureRunway = runway;
                            parsedRoute.suggestedSID = procedure;

                            parsedRoute.errors.push_back(
                                { ParsingErrorType::NO_PROCEDURE_FOUND,
                                    "Suggesting SID " + procedure.name + " for runway "
                                        + runway,
                                    0, origin, ParsingErrorLevel::INFO });

                            break;
                        }
                    }

                    if (parsedRoute.suggestedSID.has_value()) {
                        break;
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
                        parsedRoute.totalTokens - 1, destination,
                        ParsingErrorLevel::INFO });
                }
            }
        }

        // Handle STAR suggestion
        if (parsedRoute.arrivalRunway.has_value() && !waypointIds.empty()) {
            const std::string& runway = parsedRoute.arrivalRunway.value();
            const std::string& lastWaypoint = waypointIds.back();

            auto airportProcedures = NavdataObject::GetProceduresByAirport(destination);
            const auto& procedures = NavdataObject::GetProcedures();

            for (size_t idx : airportProcedures) {
                const auto& procedure = procedures[idx];
                if (procedure.type == PROCEDURE_STAR && procedure.runway == runway) {
                    for (const auto& procWpt : procedure.waypoints) {
                        if (procWpt.getIdentifier() == lastWaypoint) {
                            parsedRoute.suggestedArrivalRunway = runway;
                            parsedRoute.suggestedSTAR = procedure;

                            parsedRoute.errors.push_back(
                                { ParsingErrorType::NO_PROCEDURE_FOUND,
                                    "Suggesting STAR " + procedure.name + " for runway "
                                        + runway,
                                    parsedRoute.totalTokens - 1, destination,
                                    ParsingErrorLevel::INFO });

                            break;
                        }
                    }

                    if (parsedRoute.suggestedSTAR.has_value()) {
                        break;
                    }
                }
            }
        } else if (!parsedRoute.arrivalRunway.has_value()) {
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
                        parsedRoute.totalTokens - 1, destination,
                        ParsingErrorLevel::INFO });
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
                return parts[1];
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

        std::optional<std::string> runway = FindRunway(token);
        const std::vector<std::string> parts = absl::StrSplit(token, '/');
        std::string procedureToken = parts[0];

        bool isProcedurePattern = std::regex_match(
            procedureToken, std::regex(R"([A-Z]{3,5}\d[A-Z]?(?:.*)?(?:\/\d{2}[LRC]?)?)"));

        bool isAirportPattern = (procedureToken.length() == 4
            && std::all_of(procedureToken.begin(), procedureToken.end(),
                [](char c) { return std::isalpha(c); }));

        auto to_upper = [](const std::string& s) {
            std::string upper(s);
            std::transform(upper.begin(), upper.end(), upper.begin(),
                [](unsigned char c) { return std::toupper(c); });
            return upper;
        };

        std::vector<RouteParser::Procedure> matchingProcedures;
        if (isProcedurePattern) {
            auto procedures = NavdataObject::GetProceduresByName(procedureToken);

            for (const auto& proc : procedures) {
                if (proc.icao == anchorIcao && proc.type == type) {
                    matchingProcedures.push_back(proc);
                }
            }

            if (!matchingProcedures.empty() && runway) {
                for (const auto& procedure : matchingProcedures) {
                    if (procedure.runway == runway.value()) {
                        return FoundProcedure { procedureToken, runway, procedure };
                    }
                }
                std::string typeStr = (type == PROCEDURE_SID) ? "SID" : "STAR";
                return FoundProcedure { std::nullopt, std::nullopt, std::nullopt,
                    { ParsingError { ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
                        fmt::format("No matching runway {} found for procedure {} at {}, "
                                    "ignoring confirmed {}",
                            runway.value_or("N/A"), procedureToken, anchorIcao, typeStr),
                        tokenIndex, procedureToken, ParsingErrorLevel::PARSE_ERROR } } };
            }
        }

        if (isAirportPattern && runway) {
            if (procedureToken != anchorIcao) {
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

            return FoundProcedure { std::nullopt, runway, std::nullopt };
        }

        return FoundProcedure { std::nullopt, std::nullopt, std::nullopt };
    }
};
}; // namespace RouteParser