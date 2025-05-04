#pragma once
#include "erkir/geo/sphericalpoint.h"
#include <string>

namespace RouteParser
{
    class Runway
    {
    public:
        Runway() = default;

        inline Runway(
            const std::string& airport_ref,
            const std::string& airport_ident,
            double length_ft,
            double width_ft,
            const std::string& surface,
            bool lighted,
            bool closed,
            const std::string& le_ident,
            const erkir::spherical::Point& le_location,
            double le_elevation_ft,
            double le_heading_deg,
            double le_displaced_threshold_ft,
            const std::string& he_ident,
            const erkir::spherical::Point& he_location,
            double he_elevation_ft,
            double he_heading_deg,
            double he_displaced_threshold_ft)
            : airport_ref_(airport_ref), airport_ident_(airport_ident),
            length_ft_(length_ft), width_ft_(width_ft), surface_(surface),
            lighted_(lighted), closed_(closed), le_ident_(le_ident),
            le_location_(le_location), le_elevation_ft_(le_elevation_ft),
            le_heading_deg_(le_heading_deg), le_displaced_threshold_ft_(le_displaced_threshold_ft),
            he_ident_(he_ident), he_location_(he_location), he_elevation_ft_(he_elevation_ft),
            he_heading_deg_(he_heading_deg), he_displaced_threshold_ft_(he_displaced_threshold_ft)
        {
        }


        // Inline getters
        inline const std::string& getAirportRef() const { return airport_ref_; }
        inline const std::string& getAirportIdent() const { return airport_ident_; }
        inline double getLengthFt() const { return length_ft_; }
        inline double getWidthFt() const { return width_ft_; }
        inline const std::string& getSurface() const { return surface_; }
        inline bool isLighted() const { return lighted_; }
        inline bool isClosed() const { return closed_; }

        // Leading edge getters
        inline const std::string& getLeIdent() const { return le_ident_; }
        inline const erkir::spherical::Point& getLeLocation() const { return le_location_; }
        inline double getLeElevationFt() const { return le_elevation_ft_; }
        inline double getLeHeadingDeg() const { return le_heading_deg_; }
        inline double getLeDisplacedThresholdFt() const { return le_displaced_threshold_ft_; }

        // Higher edge getters
        inline const std::string& getHeIdent() const { return he_ident_; }
        inline const erkir::spherical::Point& getHeLocation() const { return he_location_; }
        inline double getHeElevationFt() const { return he_elevation_ft_; }
        inline double getHeHeadingDeg() const { return he_heading_deg_; }
        inline double getHeDisplacedThresholdFt() const { return he_displaced_threshold_ft_; }

    private:
        // Airport information
        std::string airport_ref_;
        std::string airport_ident_;
        double length_ft_;
        double width_ft_;
        std::string surface_;
        bool lighted_;
        bool closed_;

        // Leading edge information
        std::string le_ident_;
        erkir::spherical::Point le_location_;
        double le_elevation_ft_;
        double le_heading_deg_;
        double le_displaced_threshold_ft_;

        // Higher edge information
        std::string he_ident_;
        erkir::spherical::Point he_location_;
        double he_elevation_ft_;
        double he_heading_deg_;
        double he_displaced_threshold_ft_;
    };
}