#pragma once
#include <erkir/geo/ellipsoidalpoint.h>
#include <erkir/geo/sphericalpoint.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <variant>

namespace erkir::spherical {
inline void to_json(nlohmann::json &nlohmann_json_j,
                    const erkir::spherical::Point &nlohmann_json_t) {
  nlohmann_json_j = {nlohmann_json_t.latitude().degrees(),
                     nlohmann_json_t.longitude().degrees()};
}

inline void from_json(const nlohmann::json &nlohmann_json_j,
                      erkir::spherical::Point &nlohmann_json_t) {
  nlohmann_json_t = erkir::spherical::Point(nlohmann_json_j[0].get<double>(),
                                            nlohmann_json_j[1].get<double>());
}
} // namespace erkir::spherical

namespace nlohmann {
///////////////////////////////////////////////////////////////////////////////
// std::variant
///////////////////////////////////////////////////////////////////////////////
// Try to set the value of type T into the variant data if it fails, do nothing
template <typename T, typename... Ts>
void variant_from_json(const nlohmann::json &j, std::variant<Ts...> &data) {
  try {
    data = j.get<T>();
  } catch (...) {
  }
}

template <typename... Ts> struct adl_serializer<std::variant<Ts...>> {
  static void to_json(nlohmann::json &j, const std::variant<Ts...> &data) {
    // Will call j = v automatically for the right type
    std::visit([&j](const auto &v) { j = v; }, data);
  }

  static void from_json(const nlohmann::json &j, std::variant<Ts...> &data) {
    // Call variant_from_json for all types, only one will succeed
    (variant_from_json<Ts>(j, data), ...);
  }
};

///////////////////////////////////////////////////////////////////////////////
// std::optional
///////////////////////////////////////////////////////////////////////////////
template <typename T> struct adl_serializer<std::optional<T>> {
  static void to_json(json &j, const std::optional<T> &opt) {
    if (opt.has_value()) {
      j = *opt;
    } else {
      j = nullptr;
    }
  }

  static void from_json(const json &j, std::optional<T> &opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};

// Helper functions for working with optionals in user-defined types
template <class T>
void optional_to_json(nlohmann::json &j, const char *name,
                      const std::optional<T> &value) {
  if (value) {
    j[name] = *value;
  } else {
    j[name] = nullptr;
  }
}

template <class T>
void optional_from_json(const nlohmann::json &j, const char *name,
                        std::optional<T> &value) {
  const auto it = j.find(name);
  if (it != j.end() && !it->is_null()) {
    value = it->get<T>();
  } else {
    value = std::nullopt;
  }
}

///////////////////////////////////////////////////////////////////////////////
// all together
///////////////////////////////////////////////////////////////////////////////
template <typename> constexpr bool is_optional = false;
template <typename T> constexpr bool is_optional<std::optional<T>> = true;

template <typename T>
void extended_to_json(const char *key, nlohmann::json &j, const T &value) {
  if constexpr (is_optional<T>) {
    optional_to_json(j, key, value);
  } else {
    j[key] = value;
  }
}

template <typename T>
void extended_from_json(const char *key, const nlohmann::json &j, T &value) {
  if constexpr (is_optional<T>) {
    optional_from_json(j, key, value);
  } else if (j.contains(key)) {
    j.at(key).get_to(value);
  }
}
} // namespace nlohmann

#define EXTEND_JSON_TO(v1)                                                     \
  extended_to_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);
#define EXTEND_JSON_FROM(v1)                                                   \
  extended_from_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);

#define NLOHMANN_JSONIFY_ALL_THINGS(Type, ...)                                 \
  friend void to_json(nlohmann::json &nlohmann_json_j,                         \
                      const Type &nlohmann_json_t) {                           \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_TO, __VA_ARGS__))     \
  }                                                                            \
  friend void from_json(const nlohmann::json &nlohmann_json_j,                 \
                        Type &nlohmann_json_t) {                               \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_FROM, __VA_ARGS__))   \
  }
