#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

namespace Utils
{

  // Split string by spaces
  inline std::vector<std::string> splitBySpaces(const std::string &str)
  {
    return absl::StrSplit(str, ' ');
  }

  // Split string by a specific character
  inline std::vector<std::string> splitByChar(const std::string &str, char delimiter)
  {
    return absl::StrSplit(str, delimiter);
  }

} // namespace Utils

#endif // UTILS_H