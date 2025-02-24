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
    std::optional<std::string> runway = FindRunway(token);
    const std::vector<std::string> parts = absl::StrSplit(token, '/');
    if (parts.empty() || parts.size() < 2) {

      return FoundProcedure{
          {},
          runway,
          {},
          {ParsingError{
              ParsingErrorType::INVALID_DATA,
              fmt::format("Token split failed despite finding runway: {} {}",
                          token, icao),
              tokenIndex, token, ParsingErrorLevel::PARSE_ERROR}}};
    }

    std::string procedureToken =
        runway ? parts[0] : token; // If we have a runway, we know that the
                                   // string is split in at least 2 parts

    if (runway && procedureToken.length() == 4 && procedureToken == icao) {
      return FoundProcedure{procedureToken, runway, std::nullopt};
    }

    const auto &procedures = NavdataObject::GetProcedures();
    std::vector<RouteParser::Procedure> matchingProcedures;

    for (auto it = procedures.equal_range(procedureToken);
         it.first != it.second; ++it.first) {
      if (it.first->second.icao == icao && it.first->second.type == type) {
        matchingProcedures.push_back(it.first->second);
      }
    }

    if (matchingProcedures.empty()) {
      // No procedure found for the given token, but runway might have been
      // found
      return FoundProcedure{std::nullopt, runway, std::nullopt};
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
        runway,
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