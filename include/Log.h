#pragma once
#include <fmt/core.h>
#include <functional>

typedef std::function<void(const char *, const char *)> ILogger;

class Log {
public:
  template <typename... T>
  static void info(const std::string &message, T &&...args) {
    if (logger != nullptr) {
      logger("INFO", fmt::format(message, std::forward<T>(args)...).c_str());
    }
  }

  template <typename... T>
  static void error(const std::string &message, T &&...args) {
    if (logger != nullptr) {
      logger("ERROR", fmt::format(message, std::forward<T>(args)...).c_str());
    }
  }

  template <typename... T>
  static void warn(const std::string &message, T &&...args) {
    if (logger != nullptr) {
      logger("WARN", fmt::format(message, std::forward<T>(args)...).c_str());
    }
  }

  template <typename... T>
  static void debug(const std::string &message, T &&...args) {
    if (logger != nullptr) {
      logger("DEBUG", fmt::format(message, std::forward<T>(args)...).c_str());
    }
  }

  template <typename... T>
  static void trace(const std::string &message, T &&...args) {
    if (logger != nullptr) {
      logger("TRACE", fmt::format(message, std::forward<T>(args)...).c_str());
    }
  }

  static void SetLogger(ILogger logFunc) { logger = logFunc; }

private:
  inline static ILogger logger = nullptr;
};