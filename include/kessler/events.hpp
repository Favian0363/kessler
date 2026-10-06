#pragma once
// Turning raw hits into separate encounters, plus two small math helpers.
// No SGP4 in here, so everything can be tested with made-up numbers.

#include "kessler/pairs.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace kessler {

// All the back-to-back time steps where one pair stayed within the candidate
// threshold. One run = one encounter: the two objects come together, then
// separate again.
struct HitRun {
    std::uint32_t a{};
    std::uint32_t b{};
    int first_step{};     // first step of the run
    int last_step{};      // last step of the run
    int closest_step{};   // the step with the smallest sampled distance
    double closest_km{};  // that smallest sampled distance
};

/// Groups hits by pair and splits each pair's hits into runs of consecutive
/// steps. A gap of even one step starts a new run. Output is ordered by a,
/// then b, then first_step. Takes a copy of `hits` because it sorts them.
std::vector<HitRun> group_into_runs(std::vector<Hit> hits);

/// How far apart two objects can be AT A SAMPLE and still pass within
/// `miss_km` of each other between samples:
///     miss_km + 22.4 km/s * (step_seconds / 2) + 1 km
/// 22.4 km/s is two objects moving head-on at Earth's escape speed (11.2 km/s),
/// faster than any two Earth-orbiting objects can close on each other. Half a
/// step is the furthest any moment can be from its nearest sample. The 1 km
/// covers the orbits curving a little between samples.
/// Throws std::invalid_argument if miss_km <= 0 or step_seconds <= 0.
double candidate_threshold_km(double miss_km, double step_seconds);

/// Finds the t in [lo, hi] where f(t) is smallest, assuming f has a single
/// valley there (true for two objects passing each other in a short window).
/// Golden-section search: look at two points inside the interval, throw away
/// the side with the higher value, repeat until the interval is shorter
/// than `tol`. Each round keeps 61.8% of the interval.
/// Throws std::invalid_argument if hi < lo or tol <= 0.
double golden_section_minimize(const std::function<double(double)>& f, double lo, double hi,
                               double tol);

}  // namespace kessler
