#include "kessler/screener.hpp"

#include <cmath>
#include <stdexcept>

namespace kessler {

int sample_count(const ScreenConfig& c) {
    if (!(c.step_seconds > 0.0)) throw std::invalid_argument("step_seconds must be > 0");
    if (!(c.duration_minutes >= 0.0)) throw std::invalid_argument("duration_minutes must be >= 0");
    if (!(c.threshold_km > 0.0)) throw std::invalid_argument("threshold_km must be > 0");
    // The tiny 1e-9 stops a result like 359.99999999 (from floating-point
    // rounding) from being cut down to 359 by floor().
    return static_cast<int>(std::floor(c.duration_minutes * 60.0 / c.step_seconds + 1e-9)) + 1;
}

std::vector<Hit> screen_bruteforce(std::vector<Propagator>& objects, const ScreenConfig& config) {
    const int steps = sample_count(config);
    const std::size_t n = objects.size(); 
    std::vector<Hit> hits;

    if (n<2) return hits;

    std::vector<double> offset(n);
    std::vector<char> alive(n, 1);
    std::vector<Vec3> positions(n);

    for (std::size_t i = 0; i < n; ++i) {
        offset[i] = minutes_between(objects[i].epoch(), config.start);
    }

    for (int k = 0; k < steps; ++k){
        const double minutes_since_start = k * config.step_seconds / 60.0;

        for (std::size_t i = 0; i < n; ++i) {
            if (!alive[i]) continue;

            StateVector state;
            if (objects[i].propagate(offset[i] + minutes_since_start, state)) {
                positions[i] = Vec3{state.r_km[0], state.r_km[1], state.r_km[2]};
            } else {
                alive[i] = 0;
            }
        }

        find_close_pairs_bruteforce(positions, alive, config.threshold_km, k, hits);
    }
    return hits;
}

}  // namespace kessler
