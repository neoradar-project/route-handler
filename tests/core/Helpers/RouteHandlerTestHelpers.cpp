#include "types/ParsedRoute.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace RouteParser;

static void PRINT_ALL_PARSING_ERRORS(const ParsedRoute &parsedRoute) {
  fmt::print(fg(fmt::color::orange) | fmt::emphasis::bold,
             "--- Route Parsing Errors ---\n");
  for (const auto &error : parsedRoute.errors) {
    if (error.level == INFO) {
      fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold,
                 "INFO: ");
    } else {
      fmt::print(fg(fmt::color::dark_red) | fmt::emphasis::bold,
                 "ERROR: ");
    }
    fmt::print("{} | Token index {} | Raw token {}\n", error.message,
               error.tokenIndex, error.token);
  }
  fmt::print(fg(fmt::color::orange) | fmt::emphasis::bold,
             "--- End of Route Parsing Errors ---\n");
}

static void EXPECT_PARSE_ERROR_WITH_LEVEL(const ParsedRoute &parsedRoute,
                                          ParsingErrorLevel level,
                                          int expectedErrors) {
  const auto errors = parsedRoute.errors;
  const auto errorsWithLevel =
      std::count_if(errors.begin(), errors.end(), [level](const auto &error) {
        return error.level == level;
      });

  if (errorsWithLevel != expectedErrors) {
    PRINT_ALL_PARSING_ERRORS(parsedRoute);
  }

  if (level == ERROR) {
    const auto errorsWithLevelError =
        errorsWithLevel; // We do this code to clarify the error message
    EXPECT_EQ(errorsWithLevelError, expectedErrors);
  } else {
    const auto errorsWithLevelInfo =
        errorsWithLevel; // We do this code to clarify the error message
    EXPECT_EQ(errorsWithLevelInfo, expectedErrors);
  }
}

static void EXPECT_PARSE_ERROR_OF_TYPE(const ParsedRoute &parsedRoute,
                                       ParsingErrorType type,
                                       int expectedErrors) {
  const auto errors = parsedRoute.errors;
  const auto parsingErrorsOfType =
      std::count_if(errors.begin(), errors.end(),
                    [type](const auto &error) { return error.type == type; });

  if (parsingErrorsOfType != expectedErrors) {
    PRINT_ALL_PARSING_ERRORS(parsedRoute);
  }

  EXPECT_EQ(parsingErrorsOfType, expectedErrors);
}

static void EXPECT_BASIC_ROUTE(ParsedRoute parsedRoute) {
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::INFO, 4);
  EXPECT_PARSE_ERROR_WITH_LEVEL(parsedRoute, ParsingErrorLevel::ERROR, 0);
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_WAYPOINT,
                             2);
  EXPECT_PARSE_ERROR_OF_TYPE(parsedRoute, ParsingErrorType::UNKNOWN_PROCEDURE,
                             2);

  EXPECT_EQ(parsedRoute.SID, "TES61X");
  EXPECT_EQ(parsedRoute.departureRunway, "06");

  EXPECT_EQ(parsedRoute.STAR, "ABBEY3A");
  EXPECT_EQ(parsedRoute.arrivalRunway, "07R");
}