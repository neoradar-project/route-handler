#pragma once
#include "Navdata.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

static void ExtractNseData(const std::string& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "NSE file does not exist at path: " << filePath << std::endl;
        return;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open NSE file: " << filePath << std::endl;
            return;
        }

        nlohmann::json json;
        file >> json;
        file.close();

        std::vector<RouteParser::Waypoint> waypoints;

        // Process VORs
        if (json.contains("vor") && json["vor"].is_array()) {
            for (const auto& vor : json["vor"]) {
                try {
                    std::string name = vor["name"].get<std::string>();
                    double lat = vor["lat"].get<double>();
                    double lon = vor["lon"].get<double>();

                    int frequencyHz = 0;
                    if (vor.contains("freq") && !vor["freq"].is_null()) {
                        try {
                            frequencyHz = std::stoi(vor["freq"].get<std::string>());
                        } catch (const std::exception&) {
                        }
                    }

                    waypoints.push_back(
                        RouteParser::Waypoint(RouteParser::WaypointType::VOR, name,
                            erkir::spherical::Point(lat, lon), frequencyHz));
                } catch (const std::exception& e) {
                    std::cerr << "Error processing VOR: " << e.what() << std::endl;
                }
            }
        }

        // Process NDBs
        if (json.contains("ndb") && json["ndb"].is_array()) {
            for (const auto& ndb : json["ndb"]) {
                try {
                    std::string name = ndb["name"].get<std::string>();
                    double lat = ndb["lat"].get<double>();
                    double lon = ndb["lon"].get<double>();

                    int frequencyHz = 0;
                    if (ndb.contains("freq") && !ndb["freq"].is_null()) {
                        try {
                            frequencyHz = std::stoi(ndb["freq"].get<std::string>());
                        } catch (const std::exception&) {
                        }
                    }

                    waypoints.push_back(
                        RouteParser::Waypoint(RouteParser::WaypointType::NDB, name,
                            erkir::spherical::Point(lat, lon), frequencyHz));
                } catch (const std::exception& e) {
                    std::cerr << "Error processing NDB: " << e.what() << std::endl;
                }
            }
        }

        // Process FIXes
        if (json.contains("fix") && json["fix"].is_array()) {
            for (const auto& fix : json["fix"]) {
                try {
                    std::string name = fix["name"].get<std::string>();
                    double lat = fix["lat"].get<double>();
                    double lon = fix["lon"].get<double>();

                    waypoints.push_back(
                        RouteParser::Waypoint(RouteParser::WaypointType::FIX, name,
                            erkir::spherical::Point(lat, lon)));
                } catch (const std::exception& e) {
                    std::cerr << "Error processing FIX: " << e.what() << std::endl;
                }
            }
        }

        // Process airports
        if (json.contains("airport") && json["airport"].is_array()) {
            for (const auto& airport : json["airport"]) {
                try {
                    std::string name = airport["name"].get<std::string>();
                    double lat = airport["lat"].get<double>();
                    double lon = airport["lon"].get<double>();

                    waypoints.push_back(
                        RouteParser::Waypoint(RouteParser::WaypointType::AIRPORT, name,
                            erkir::spherical::Point(lat, lon)));
                } catch (const std::exception& e) {
                    std::cerr << "Error processing airport: " << e.what() << std::endl;
                }
            }
        }

        // Load the extracted waypoints
        if (!waypoints.empty()) {
            std::cout << "Loading " << waypoints.size() << " NSE waypoints." << std::endl;
            RouteParser::NavdataObject::LoadNseWaypoints(
                waypoints, "NSE Provider - " + filePath);
        }

        // Process procedures
        std::vector<RouteParser::Procedure> procedures;

        if (json.contains("procedure") && json["procedure"].is_array()) {
            for (const auto& obj : json["procedure"]) {
                if (!obj.contains("type") || !obj.contains("icao")
                    || !obj.contains("name") || !obj.contains("points")
                    || !obj["points"].is_array()) {
                    std::cerr << "Skipping invalid procedure entry in NSE file."
                              << std::endl;
                    continue;
                }

                RouteParser::Procedure procedure;
                procedure.name = obj["name"].get<std::string>();
                procedure.icao = obj["icao"].get<std::string>();

                if (obj.contains("runway") && !obj["runway"].is_null()) {
                    procedure.runway = obj["runway"].get<std::string>();
                }

                std::string type = obj["type"].get<std::string>();
                procedure.type = (type == "SID")
                    ? RouteParser::ProcedureType::PROCEDURE_SID
                    : RouteParser::ProcedureType::PROCEDURE_STAR;

                auto airportReference = RouteParser::NavdataObject::FindWaypointByType(
                    procedure.icao, WaypointType::AIRPORT);

                for (const auto& point : obj["points"]) {
                    if (!point.is_string()) {
                        continue;
                    }

                    std::string pointName = point.get<std::string>();
                    auto closestWaypoint
                        = RouteParser::NavdataObject::FindClosestWaypointTo(
                            pointName, airportReference);

                    if (closestWaypoint.has_value()) {
                        procedure.waypoints.push_back(closestWaypoint.value());
                    } else {
                        std::cout << "Warning: Could not find waypoint '" << pointName
                                  << "' for procedure '" << procedure.name << "'"
                                  << std::endl;
                    }
                }

                procedures.push_back(procedure);
            }
        }

        // Set the procedures
        if (!procedures.empty()) {
            std::cout << "Loaded " << procedures.size() << " procedures from NSE file."
                      << std::endl;
            RouteParser::NavdataObject::SetProcedures(procedures);
        } else {
            std::cout << "No procedures found in NSE file." << std::endl;
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error processing NSE file: " << e.what() << std::endl;
    }
}