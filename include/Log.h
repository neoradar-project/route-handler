#pragma once
#include <fmt/core.h>
#include <functional>

typedef std::function<void(const char *, const char *)> ILogger;

class Log
{
public:
  template <typename... Args>
  static void info(fmt::format_string<Args...> message, Args &&...args)
  {
    log("INFO", message, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void error(fmt::format_string<Args...> message, Args &&...args)
  {
    log("ERROR", message, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(fmt::format_string<Args...> message, Args &&...args)
  {
    log("WARN", message, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void debug(fmt::format_string<Args...> message, Args &&...args)
  {
    log("DEBUG", message, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void trace(fmt::format_string<Args...> message, Args &&...args)
  {
    log("TRACE", message, std::forward<Args>(args)...);
  }

  static void SetLogger(ILogger logFunc) { logger = logFunc; }

private:
  template <typename... Args>
  static void log(const char *level, fmt::format_string<Args...> message, Args &&...args)
  {
    if (logger != nullptr)
    {
      logger(level, fmt::format(message, std::forward<Args>(args)...).c_str());
    }
  }

  inline static ILogger logger = nullptr;
};