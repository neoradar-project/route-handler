#pragma once
#include "Navdata.h"
#include "Utils.h"
#include "fmt/core.h"
#include "types/ParsedRoute.h"
#include "types/Procedure.h"
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
  static std::optional<std::string> FindRunway(std::string token) {

    if (token.find('/') != std::string::npos) {
      auto parts = Utils::splitByChar(token, '/');
      if (parts[1].size() == 2 || parts[1].size() == 3) {
        return parts[1]; // Found a runway
      }
    }

    return std::nullopt;
  }

  static FoundProcedure FindProcedure(std::string token, std::string icao,
                                      ProcedureType type) {
    std::optional<std::string> runway = FindRunway(token);
    if (runway.has_value()) {
      token = Utils::splitByChar(token, '/')[0];
    }

    if (token.length() == 4) {
      // It's an ICAO code, not a SID, so return
      return FoundProcedure{Utils::splitByChar(token, '/')[1], runway};
    }

    // Check if it's a procedure
    std::vector<RouteParser::Procedure> matchingProcedures;

    if (NavdataContainer->GetProcedures().find(token) !=
        NavdataContainer->GetProcedures().end()) {
      auto procedures = NavdataContainer->GetProcedures().equal_range(token);
      for (auto it = procedures.first; it != procedures.second; ++it) {
        if (it->second.icao == icao && it->second.type == type) {
          matchingProcedures.push_back(it->second);
        }
      }
    }

    // We now check the matched procedures, and we will see if we can find a
    // matching runway
    if (matchingProcedures.size() == 0) {
      // We didn't find any procedures, so we return whatever text we have with
      // an info, this could be because we simply do not have this procedure in
      // dataset
      return FoundProcedure{
          token,
          runway,
          std::nullopt,
          {ParsingError{ParsingErrorType::UNKNOWN_PROCEDURE,
                        fmt::format("No matching procedure found for {} at {}",
                                    token, icao),
                        0, token, ParsingErrorLevel::INFO}}};
    }

    // If we have a runway, we check for a match
    if (runway.has_value()) {
      for (auto &procedure : matchingProcedures) {
        if (procedure.runway == runway) {
          return FoundProcedure{token, runway, procedure};
        }
      }

      // We didn't find a matching runway, so we return the first procedure with
      // an error
      return FoundProcedure{
          token,
          runway,
          matchingProcedures[0],
          {ParsingError{ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
                        fmt::format("No matching runway {} found for {} at {}",
                                    runway.value_or("N/A"), token, icao),
                        0, token, ParsingErrorLevel::ERROR}}};
    }

    // If we don't have anything at all, we return the first token and hopefully a found runway
    return FoundProcedure{token, runway};
  }
};
} // namespace RouteParser
