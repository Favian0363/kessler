#include "kessler/propagator.hpp"

#include <cstdio>
#include <string>

namespace kessler {

Propagator::Propagator(const Sgp4Elements& e, Sgp4Mode mode)
    : epoch_(e.epoch), catalog_number_(e.catalog_number) {
    char satn[16] = {};
    std::snprintf(satn, sizeof satn, "%05d", e.catalog_number);

    // TLEs are generated with WGS-72 constants, so propagation must use WGS-72.
    const bool ok = SGP4Funcs::sgp4init(
        wgs72, static_cast<char>(mode), satn, e.epoch_days_since_1950, e.bstar, e.ndot, e.nddot,
        e.eccentricity, e.arg_perigee_rad, e.inclination_rad, e.mean_anomaly_rad,
        e.mean_motion_rad_per_min, e.raan_rad, rec_);

    if (!ok || rec_.error != 0) {
        throw Sgp4InitError("SGP4 init failed for object " + std::to_string(e.catalog_number) +
                                " (error " + std::to_string(rec_.error) + ")",
                            rec_.error);
    }
    
    rec_.jdsatepoch = e.epoch.day;
    rec_.jdsatepochF = e.epoch.frac;
}

bool Propagator::propagate(double minutes_since_epoch, StateVector& out) {
    const bool ok = SGP4Funcs::sgp4(rec_, minutes_since_epoch, out.r_km.data(), out.v_km_s.data());
    return ok && rec_.error == 0;
}

} // namespace kessler
