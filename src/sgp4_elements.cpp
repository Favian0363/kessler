#include "kessler/sgp4_elements.hpp"

#include <numbers>

namespace kessler {
namespace {
constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kMinPerDay = 1440.0;
// rev/day -> rad/min. Vallado writes this as dividing by xpdotp = 1440 / (2*pi).
constexpr double kRevPerDayToRadPerMin = 2.0 * std::numbers::pi / kMinPerDay;
constexpr double kJd1950Jan0 = 2433281.5;
} // namespace

Sgp4Elements to_sgp4_elements(const Tle& t) {
    Sgp4Elements e;
    e.catalog_number = t.catalog_number;
    e.epoch = tle_epoch_jd(t.epoch_year, t.epoch_day);
    e.epoch_days_since_1950 = (e.epoch.day + e.epoch.frac) - kJd1950Jan0;
    e.bstar = t.bstar;
    e.ndot  = t.ndot_over_2 * kRevPerDayToRadPerMin / kMinPerDay;
    e.nddot = t.nddot_over_6 * kRevPerDayToRadPerMin / (kMinPerDay * kMinPerDay);
    e.eccentricity     = t.eccentricity;
    e.arg_perigee_rad  = t.arg_perigee_deg * kDegToRad;
    e.inclination_rad  = t.inclination_deg * kDegToRad;
    e.mean_anomaly_rad = t.mean_anomaly_deg * kDegToRad;
    e.raan_rad         = t.raan_deg * kDegToRad;
    e.mean_motion_rad_per_min = t.mean_motion_rev_per_day * kRevPerDayToRadPerMin;
    return e;
}

} // namespace kessler
