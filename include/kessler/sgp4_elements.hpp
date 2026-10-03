#pragma once
// TLE fields -> the units Vallado's sgp4init() expects.
//
// These conversions mirror Vallado's own twoline2rv(), so feeding these
// numbers to sgp4init() must reproduce the reference results exactly.
// That equivalence is tested once the reference code is in the build.

#include "kessler/time.hpp"
#include "kessler/tle.hpp"

namespace kessler {

struct Sgp4Elements {
    int        catalog_number{};
    JulianDate epoch{};
    double epoch_days_since_1950{};    // sgp4init's "epoch": JD - 2433281.5 (1950 Jan 0.0 UTC)
    double bstar{};                    // drag term [1/earth radii], unchanged from the TLE
    double ndot{};                     // TLE ndot/2 field in rad/min^2 (stored, not used by SGP4)
    double nddot{};                    // TLE nddot/6 field in rad/min^3 (stored, not used by SGP4)
    double eccentricity{};
    double arg_perigee_rad{};
    double inclination_rad{};
    double mean_anomaly_rad{};
    double mean_motion_rad_per_min{};  // Kozai mean motion; SGP4 converts it to Brouwer internally
    double raan_rad{};
};

Sgp4Elements to_sgp4_elements(const Tle& tle);

} // namespace kessler
