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

std::vector<Conjunction> refine_runs(const std::vector<Propagator>& objects,
                                     const ScreenConfig& config, const std::vector<HitRun>& runs,
                                     double miss_km, bool skip_impossible, RefineStats* stats) {
    if (!(miss_km > 0.0)) throw std::invalid_argument("miss_km must be > 0");
    const int last_step = sample_count(config) - 1;  // also validates the config
    const double step_minutes = config.step_seconds / 60.0;

    std::vector<double> offset(objects.size());
    for (std::size_t i = 0; i < objects.size(); ++i) {
        offset[i] = minutes_between(objects[i].epoch(), config.start);
    }

    std::vector<Conjunction> out;
    std::size_t refined = 0, skipped = 0;
    const auto count = static_cast<std::int64_t>(runs.size());

#pragma omp parallel
    {
        std::vector<Conjunction> local;
        std::size_t local_refined = 0, local_skipped = 0;

#pragma omp for schedule(dynamic, 1024) nowait
        for (std::int64_t r = 0; r < count; ++r) {
            const HitRun& run = runs[static_cast<std::size_t>(r)];
            // Private copies: SGP4 changes its internal state when it runs, so
            // two threads must never use the same Propagator object.
            Propagator pa = objects[run.a];
            Propagator pb = objects[run.b];
            const double off_a = offset[run.a];
            const double off_b = offset[run.b];

            if (skip_impossible) {
                const double t = run.closest_step * step_minutes;
                StateVector sa, sb;
                if (pa.propagate(off_a + t, sa) && pb.propagate(off_b + t, sb)) {
                    const double rel_speed = length_of_difference(sa.v_km_s, sb.v_km_s);
                    if (!can_get_within(run.closest_km, rel_speed, config.step_seconds, miss_km)) {
                        ++local_skipped;
                        continue;
                    }
                }
            }
            ++local_refined;

            // Distance at t minutes after the start; "infinitely far" if SGP4 fails.
            const auto distance_at = [&](double t) {
                StateVector sa, sb;
                if (!pa.propagate(off_a + t, sa) || !pb.propagate(off_b + t, sb)) {
                    return std::numeric_limits<double>::max();
                }
                return length_of_difference(sa.r_km, sb.r_km);
            };

            // The true closest moment is within one step of the closest sample,
            // clamped to the screening window.
            const double lo = std::max(run.closest_step - 1, 0) * step_minutes;
            const double hi = std::min(run.closest_step + 1, last_step) * step_minutes;
            const double tca = golden_section_minimize(distance_at, lo, hi, kTimeToleranceMinutes);

            StateVector sa, sb;
            if (!pa.propagate(off_a + tca, sa) || !pb.propagate(off_b + tca, sb)) continue;
            const double miss = length_of_difference(sa.r_km, sb.r_km);
            if (miss < miss_km) {
                local.push_back({run.a, run.b, tca, miss, length_of_difference(sa.v_km_s, sb.v_km_s)});
            }
        }

#pragma omp critical
        {
            out.insert(out.end(), local.begin(), local.end());
            refined += local_refined;
            skipped += local_skipped;
        }
    }

    std::sort(out.begin(), out.end(), [](const Conjunction& x, const Conjunction& y) {
        if (x.tca_minutes != y.tca_minutes) return x.tca_minutes < y.tca_minutes;
        if (x.a != y.a) return x.a < y.a;
        return x.b < y.b;
    });
    if (stats != nullptr) *stats = {refined, skipped};
    return out;
}

}  // namespace kessler
