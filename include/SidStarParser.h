#pragma once
#include "Navdata.h"
#include "absl/strings/str_split.h"
#include "fmt/core.h"
#include "types/ParsingError.h"
#include "types/Procedure.h"
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

namespace RouteParser
{

  struct FoundProcedure
  {
    std::optional<std::string> procedure;
    std::optional<std::string> runway;
    std::optional<RouteParser::Procedure> extractedProcedure;
    std::vector<RouteParser::ParsingError> errors;
  };

  class SidStarParser
  {
  public:
    static std::optional<std::string> FindRunway(const std::string &token)
    {
      auto slashPos = token.find('/');
      if (slashPos != std::string::npos)
      {
        const std::vector<std::string> parts = absl::StrSplit(token, '/');
        if (parts.size() > 1 && (parts[1].size() == 2 || parts[1].size() == 3))
        {
          return parts[1]; // Found a runway
        }
      }
      return std::nullopt;
    }

    // Helper function to extract the numeric and letter suffix
    static std::pair<std::string, std::string> ExtractComponents(const std::string &identifier)
    {
      std::string prefix;
      std::string suffix;

      bool numericStarted = false;
      for (char c : identifier)
      {
        if (!numericStarted && std::isalpha(c))
        {
          prefix += c;
        }
        else
        {
          numericStarted = true;
          suffix += c;
        }
      }

      return {prefix, suffix};
    }

    // Helper function to check if two procedure names potentially match
    static bool AreProceduresRelated(const std::string &proc1, const std::string &proc2)
    {
      auto [prefix1, suffix1] = ExtractComponents(proc1);
      auto [prefix2, suffix2] = ExtractComponents(proc2);

      // Check if the prefixes are similar (allowing for one character difference)
      if (prefix1.length() == prefix2.length() ||
          std::abs(static_cast<int>(prefix1.length() - prefix2.length())) == 1)
      {
        int differences = 0;
        size_t maxLen = std::max(prefix1.length(), prefix2.length());
        for (size_t i = 0; i < maxLen; i++)
        {
          char c1 = i < prefix1.length() ? prefix1[i] : 0;
          char c2 = i < prefix2.length() ? prefix2[i] : 0;
          if (c1 != c2)
            differences++;
        }
        // Allow up to one character difference in the prefix
        if (differences <= 1)
        {
          // If suffixes match exactly, it's likely the same procedure
          return suffix1 == suffix2;
        }
      }
      return false;
    }

    static FoundProcedure FindProcedure(const std::string &token,
                                        const std::string &icao,
                                        ProcedureType type,
                                        int tokenIndex)
    {
      std::optional<std::string> runway = FindRunway(token);
      const std::vector<std::string> parts = absl::StrSplit(token, '/');
      std::string procedureToken = runway ? parts[0] : token;

      if (runway && procedureToken.length() == 4)
      {
        if (procedureToken == icao)
        {
          return FoundProcedure{procedureToken, runway, std::nullopt, {}};
        }
        else
        {
          return FoundProcedure{
              std::nullopt,
              std::nullopt,
              std::nullopt,
              {ParsingError{
                  ParsingErrorType::INVALID_RUNWAY,
                  fmt::format("Expected runway for {} but found a runway for {}",
                              icao, procedureToken),
                  tokenIndex,
                  token,
                  ParsingErrorLevel::ERROR}}};
        }
      }

      const auto &procedures = NavdataObject::GetProcedures();
      std::vector<RouteParser::Procedure> matchingProcedures;

      // Find all procedures that could match
      for (auto it = procedures.begin(); it != procedures.end(); ++it)
      {
        const auto &procedure = it->second;
        if (procedure.icao == icao && procedure.type == type)
        {
          if (procedure.name == procedureToken ||
              AreProceduresRelated(procedure.name, procedureToken))
          {
            matchingProcedures.push_back(procedure);
          }
        }
      }

      if (matchingProcedures.empty())
      {
        return FoundProcedure{
            procedureToken,
            runway,
            std::nullopt,
            {ParsingError{
                ParsingErrorType::UNKNOWN_PROCEDURE,
                fmt::format("No matching procedure found for {} at {}",
                            procedureToken, icao),
                tokenIndex,
                procedureToken,
                ParsingErrorLevel::INFO}}};
      }

      if (runway)
      {
        for (const auto &procedure : matchingProcedures)
        {
          if (procedure.runway == runway)
          {
            return FoundProcedure{procedure.name, runway, procedure};
          }
        }

        return FoundProcedure{
            matchingProcedures[0].name,
            runway,
            matchingProcedures[0],
            {ParsingError{
                ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
                fmt::format("No matching runway {} found for procedure {} at {}",
                            runway.value_or("N/A"), procedureToken, icao),
                tokenIndex,
                procedureToken,
                ParsingErrorLevel::ERROR}}};
      }

      return FoundProcedure{matchingProcedures[0].name, runway, matchingProcedures[0]};
    }
  };

} // namespace RouteParser