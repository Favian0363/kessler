#include "kessler/fast_screener.hpp"

#include "kessler/grid.hpp"

#include <cstdint>

namespace kessler {

std::vector<Hit> screen_grid(std::vector<Propagator>& objects, const ScreenConfig& config) {
    const int steps = sample_count(config);  // also validates the config
    const std::size_t n = objects.size();

    std::vector<double> offset(n);
    for (std::size_t i = 0; i < n; ++i) {
        offset[i] = minutes_between(objects[i].epoch(), config.start);
    }

    std::vector<char> alive(n, 1);
    std::vector<Vec3> positions(n);
    std::vector<Hit> hits;
    const auto count = static_cast<std::int64_t>(n);

    for (int k = 0; k < steps; ++k) {
        const double minutes_since_start = k * config.step_seconds / 60.0;

        // Threads split the objects between them. Each object is handled by
        // exactly one thread, so no two threads ever touch the same Propagator
        // (SGP4 changes its internal state when it runs).
#pragma omp parallel for schedule(dynamic, 64)
        for (std::int64_t ii = 0; ii < count; ++ii) {
            const auto i = static_cast<std::size_t>(ii);
            if (!alive[i]) continue;
            StateVector state;
            if (objects[i].propagate(offset[i] + minutes_since_start, state)) {
                positions[i] = Vec3{state.r_km[0], state.r_km[1], state.r_km[2]};
            } else {
                alive[i] = 0;
            }
        }

        find_close_pairs_grid(positions, alive, config.threshold_km, k, hits);
    }
    return hits;
}

}  // namespace kessler
