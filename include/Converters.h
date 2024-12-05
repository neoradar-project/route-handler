#pragma once

#include <erkir/geo/ellipsoidalpoint.h>
#include <erkir/geo/sphericalpoint.h>
#include <nlohmann/json.hpp>

namespace erkir::spherical {
inline void to_json(
    nlohmann ::json& nlohmann_json_j, const erkir::spherical::Point& nlohmann_json_t)
{
    nlohmann_json_j
        = { nlohmann_json_t.latitude().degrees(), nlohmann_json_t.longitude().degrees() };
}
inline void from_json(
    const nlohmann ::json& nlohmann_json_j, erkir::spherical::Point& nlohmann_json_t)
{
    nlohmann_json_t = erkir::spherical::Point(
        nlohmann_json_j[0].get<double>(), nlohmann_json_j[1].get<double>());
};
}

namespace nlohmann {
template<typename T> struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt)
    {
        if (opt.has_value()) {
            j = *opt;
        } else {
            j = nullptr;
        }
    }

    static void from_json(const json& j, std::optional<T>& opt)
    {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }
};
}