#pragma once
#include "Navdata.h"
#include "absl/strings/str_split.h"
#include "fmt/core.h"
#include "types/ParsingError.h"
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
  static std::optional<std::string> FindRunway(const std::string &token) {
    auto slashPos = token.find('/');
    if (slashPos != std::string::npos) {
      const std::vector<std::string> parts = absl::StrSplit(token, '/');
      if (parts.size() > 1 && (parts[1].size() == 2 || parts[1].size() == 3)) {
        return parts[1]; // Found a runway
      }
    }
    return std::nullopt;
  }

  static FoundProcedure FindProcedure(const std::string &token,
                                      const std::string &icao,
                                      ProcedureType type, int tokenIndex) {
    if (token.empty() || icao.empty()) {
      return FoundProcedure{
          {},
          {},
          {},
          {ParsingError{ParsingErrorType::INVALID_DATA, "Empty token or ICAO",
                        tokenIndex, token}}};
    }

    std::optional<std::string> runway = FindRunway(token);
    const std::vector<std::string> parts = absl::StrSplit(token, '/');

    // Handle single token case properly
    std::string procedureToken = token;
    if (!parts.empty() && parts.size() >= 2) {
      procedureToken = parts[0];
    }

    // Handle direct ICAO+runway case
    if (runway && procedureToken.length() == 4 && procedureToken == icao) {
      return FoundProcedure{std::nullopt, runway, std::nullopt};
    }

    // Safe procedures access
    const auto &procedures = NavdataObject::GetProcedures();
    std::vector<RouteParser::Procedure> matchingProcedures;

    auto range = procedures.equal_range(procedureToken);
    for (auto it = range.first; it != range.second; ++it) {
      if (it->second.icao == icao && it->second.type == type) {
        matchingProcedures.push_back(it->second);
      }
    }

    if (matchingProcedures.empty()) {
      // No procedure found for the given token, and runway has been checked against icaos
      return FoundProcedure{std::nullopt, std::nullopt, std::nullopt};
    }

    // There is no runway but matching procedures
    if (!runway) {
      return FoundProcedure{procedureToken, runway, matchingProcedures[0]};
    }

    if (runway) {
      // There is a runway, there are matching procedure, let's try and catch
      // the right one
      for (const auto &procedure : matchingProcedures) {
        if (procedure.runway == runway) {
          // Full, valid match for procedure + runway
          return FoundProcedure{procedureToken, runway, procedure};
        }
      }
    }

    // No matching runway found for procedure, return the runway with an error
    return FoundProcedure{
        procedureToken,
        {},
        matchingProcedures[0],
        {ParsingError{
            ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
            fmt::format("No matching runway {} found for procedure {} at {}, "
                        "returning first matching procedure",
                        runway.value_or("N/A"), procedureToken, icao),
            tokenIndex, procedureToken, ParsingErrorLevel::PARSE_ERROR}}};
  }
};
}; // namespace RouteParser