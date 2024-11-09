#pragma once
#include "Navdata.h"
#include "Utils.h"
#include "fmt/core.h"
#include "types/ParsedRoute.h"
#include "types/Procedure.h"
#include <optional>
#include <string>
#include <vector>

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
        auto parts = Utils::splitByChar(token, '/');
        if (parts.size() > 1 && (parts[1].size() == 2 || parts[1].size() == 3))
        {
          return parts[1]; // Found a runway
        }
      }
      return std::nullopt;
    }

    static FoundProcedure FindProcedure(const std::string &token,
                                        const std::string &icao,
                                        ProcedureType type)
    {
      std::optional<std::string> runway = FindRunway(token);
      std::string procedureToken = token;

      if (runway.has_value())
      {
        auto parts = Utils::splitByChar(token, '/');
        if (!parts.empty())
        {
          procedureToken = parts[0];
        }
      }

      if (procedureToken.length() == 4)
      {
        // It's an ICAO code, not a SID, so return
        auto parts = Utils::splitByChar(token, '/');
        return FoundProcedure{
            parts.size() > 1 ? std::optional<std::string>{parts[1]} : std::nullopt,
            runway,
            std::nullopt,
            {}};
      }

      // Get a reference to the procedures container
      const auto &procedures = NavdataContainer->GetProcedures();

      // Find matching procedures
      std::vector<RouteParser::Procedure> matchingProcedures;

      auto range = procedures.equal_range(procedureToken);
      for (auto it = range.first; it != range.second; ++it)
      {
        if (it->second.icao == icao && it->second.type == type)
        {
          matchingProcedures.push_back(it->second);
        }
      }

      // Handle case where no procedures were found
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
                0,
                procedureToken,
                ParsingErrorLevel::INFO}}};
      }

      // If we have a runway, look for a matching procedure
      if (runway.has_value())
      {
        for (const auto &procedure : matchingProcedures)
        {
          if (procedure.runway == runway)
          {
            return FoundProcedure{procedureToken, runway, procedure};
          }
        }

        // No matching runway found, return first procedure with error
        return FoundProcedure{
            procedureToken,
            runway,
            matchingProcedures[0],
            {ParsingError{
                ParsingErrorType::PROCEDURE_RUNWAY_MISMATCH,
                fmt::format("No matching runway {} found for {} at {}",
                            runway.value_or("N/A"), procedureToken, icao),
                0,
                procedureToken,
                ParsingErrorLevel::ERROR}}};
      }

      // Return first procedure if no runway specified
      return FoundProcedure{procedureToken, runway};
    }
  };

} // namespace RouteParser