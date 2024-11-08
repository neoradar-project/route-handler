#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace Utils {

// Trim leading and trailing whitespaces
inline std::string trim(const std::string &str) {
  auto start = str.begin();
  while (start != str.end() && std::isspace(*start)) {
    start++;
  }

  auto end = str.end();
  do {
    end--;
  } while (std::distance(start, end) > 0 && std::isspace(*end));

  return std::string(start, end + 1);
}

// Reduce multiple spaces to a single space
inline std::string reduceSpaces(const std::string &str) {
  std::istringstream iss(str);
  std::string result, word;
  while (iss >> word) {
    if (!result.empty()) {
      result += " ";
    }
    result += word;
  }
  return result;
}

// Combined function to trim and reduce spaces
inline std::string trimAndReduceSpaces(const std::string &str) {
  return reduceSpaces(trim(str));
}

// Remove all occurrences of a character from a string
inline std::string removeChar(const std::string &str, char ch) {
  std::string result;
  std::remove_copy(str.begin(), str.end(), std::back_inserter(result), ch);
  return result;
}

// Split string by spaces
inline std::vector<std::string> splitBySpaces(const std::string &str) {
    std::istringstream iss(str);
    std::vector<std::string> result;
    std::string word;
    while (iss >> word) {
        result.push_back(word);
    }
    return result;
}

// Split string by a specific character
inline std::vector<std::string> splitByChar(const std::string &str, char delimiter) {
  std::vector<std::string> result;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    result.push_back(token);
  }
  return result;
}

} // namespace Utils

#endif // UTILS_H