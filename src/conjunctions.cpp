#include "kessler/conjunctions.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace kessler {
namespace {

double length_of_difference(const std::array<double, 3>& p, const std::array<double, 3>& q) {
    const double dx = p[0] - q[0];
    const double dy = p[1] - q[1];
    const double dz = p[2] - q[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Stop when the search window is shorter than this: 1e-6 minutes = 60 microseconds.
constexpr double kTimeToleranceMinutes = 1e-6;

}  // namespace

std::vector<Conjunction> refine_runs(std::vector<Propagator>& objects, const ScreenConfig& config,
                                     const std::vector<HitRun>& runs, double miss_km) {
    if (!(miss_km > 0.0)) throw std::invalid_argument("miss_km must be > 0");
    const int last_step = sample_count(config) - 1;  // also validates the config
    const double step_minutes = config.step_seconds / 60.0;

    // Same idea as in screen_bruteforce: minutes from each object's own epoch to the start.
    std::vector<double> offset(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        offset[i] = minutes_between(objects[i].epoch(), config.start);
    }

    std::vector<Conjunction> out;
    for (const HitRun& run : runs) {
        Propagator& pa = objects[run.a];
        Propagator& pb = objects[run.b];
        const double off_a = offset[run.a];
        const double off_b = offset[run.b];

        // Distance between the two objects at t minutes after the start.
        // If SGP4 fails for either one, report "infinitely far" so the search avoids it.
        const auto distance_at = [&](double t) {
            StateVector sa, sb;
            if (!pa.propagate(off_a + t, sa) || !pb.propagate(off_b + t, sb)) {
                return std::numeric_limits<double>::max();
            }
            return length_of_difference(sa.r_km, sb.r_km);
        };

        // The true closest moment is within one step of the closest sample.
        // Clamp to the screening window so we never look before the start or after the end.
        const double lo = std::max(run.closest_step - 1, 0) * step_minutes;
        const double hi = std::min(run.closest_step + 1, last_step) * step_minutes;
        const double tca = golden_section_minimize(distance_at, lo, hi, kTimeToleranceMinutes);

        StateVector sa, sb;
        if (!pa.propagate(off_a + tca, sa) || !pb.propagate(off_b + tca, sb)) continue;
        const double miss = length_of_difference(sa.r_km, sb.r_km);
        if (miss < miss_km) {
            out.push_back({run.a, run.b, tca, miss, length_of_difference(sa.v_km_s, sb.v_km_s)});
        }
    }

    std::sort(out.begin(), out.end(), [](const Conjunction& x, const Conjunction& y) {
        if (x.tca_minutes != y.tca_minutes) return x.tca_minutes < y.tca_minutes;
        if (x.a != y.a) return x.a < y.a;
        return x.b < y.b;
    });
    return out;
}

}  // namespace kessler
